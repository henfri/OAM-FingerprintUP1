#include "FingerprintModule.h"

ActionChannel::ActionChannel(uint8_t index, Fingerprint finger)
{
    _channelIndex = index;
    _finger = finger;
}

const std::string ActionChannel::name()
{
    return "Action";
}

void ActionChannel::loop()
{
    if (_actionCallResetTime > 0 && delayCheck(_actionCallResetTime, ParamFIN_AuthDelayTimeMS))
        resetActionCall();

    if (_stairLightTime > 0 && delayCheck(_stairLightTime, ParamFIN_aDelayTimeMS))
    {
        KoFIN_aSwitch.value(false, DPT_Switch);
        _stairLightTime = 0;
    }
    processReadRequests();
}

void ActionChannel::processInputKo(GroupObject &ko)
{
    switch (FIN_KoCalcIndex(ko.asap()))
    {
        case FIN_KoaCall:
            if (ko.value(DPT_Switch))
            {
                _authenticateActive = true;
                _actionCallResetTime = delayTimerInit();
                _finger.setLed(Fingerprint::State::WaitForFinger);
            }
            break;
    }
}

bool ActionChannel::processScan(uint16_t location)
{
    if (_authenticateActive && !ParamFIN_aAuthenticate)
        return false;

    if (!ParamFIN_aAuthenticate || KoFIN_aCall.value(DPT_Switch))
    {
        switch (ParamFIN_aActionType)
        {
            case 0: // action deactivated
                break;
            case 1: // switch
                KoFIN_aSwitch.value(ParamFIN_aOnOff, DPT_Switch);
                break;
            case 2: // toggle
                KoFIN_aSwitch.value(!KoFIN_aState.value(DPT_Switch), DPT_Switch);
                KoFIN_aState.value(KoFIN_aSwitch.value(DPT_Switch), DPT_Switch);
                break;
            case 3: // stair light
                KoFIN_aSwitch.value(true, DPT_Switch);
                _stairLightTime = delayTimerInit();
                break;
        }

        if (KoFIN_aCall.value(DPT_Switch))
        {
            KoFIN_aCall.value(false, DPT_Switch);
            _actionCallResetTime = 0;
            _authenticateActive = false;
        }

        return true;
    }

    return false;
}

void ActionChannel::processReadRequests()
{
    // we send a read request just once per channel
    if (_readRequestSent) return;

    // is there a state field to read?
    if (ParamFIN_aActionType == 2) // toggle
        _readRequestSent = openknxFingerprintModule.sendReadRequest(KoFIN_aState);
    else
        _readRequestSent = true;
}

void ActionChannel::resetActionCall()
{
    _actionCallResetTime = 0;
    if (!_authenticateActive)
        return;

    KoFIN_aCall.value(false, DPT_Switch);
    _finger.setLed(Fingerprint::State::None);
    _authenticateActive = false;
}
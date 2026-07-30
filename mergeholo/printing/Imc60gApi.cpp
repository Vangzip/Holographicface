#include "Imc60gApi.h"

#include "IMC_Library.h"
#include "errorcode.h"

namespace {

short cardNumber(unsigned int cardIndex)
{
    return static_cast<short>(cardIndex);
}

} // namespace

Imc60gApi::Imc60gApi()
{
    profileVelocities_.fill(0.0);
}

int Imc60gApi::getCardsNum(unsigned int* count)
{
    short nativeCount = 0;
    short cardIndexes[4] = {0, 0, 0, 0};
    const int rc = IMC_GetCardsNum(&nativeCount, cardIndexes);
    if (rc == 0 && count) {
        *count = nativeCount > 0 ? static_cast<unsigned int>(nativeCount) : 0U;
    }
    return rc;
}

int Imc60gApi::openCard(unsigned int cardIndex)
{
    return IMC_OpenCard(cardNumber(cardIndex));
}

int Imc60gApi::closeCard(unsigned int cardIndex)
{
    return IMC_CloseCard(cardNumber(cardIndex));
}

int Imc60gApi::scanEthercat(unsigned int cardIndex, short waitSeconds)
{
    return IMC_ScanCardEcat(cardNumber(cardIndex), waitSeconds);
}

int Imc60gApi::initEthercat(unsigned int cardIndex)
{
    return IMC_InitEcatComm(cardNumber(cardIndex));
}

int Imc60gApi::startEthercat(unsigned int cardIndex)
{
    return IMC_StartEcatComm(cardNumber(cardIndex));
}

int Imc60gApi::stopEthercat(unsigned int cardIndex)
{
    return IMC_DelEcatComm(cardNumber(cardIndex));
}

int Imc60gApi::setEmergencyLevel(unsigned int cardIndex, short inverted)
{
    return IMC_SetEmgTrigLevelInv(cardNumber(cardIndex), inverted);
}

int Imc60gApi::clearAxisStatus(unsigned int cardIndex, short axis)
{
    return IMC_ClrAxSts(cardNumber(cardIndex), axis, 1);
}

int Imc60gApi::servoOn(unsigned int cardIndex, short axis)
{
    return IMC_ServoOn(cardNumber(cardIndex), axis, 1);
}

int Imc60gApi::servoOff(unsigned int cardIndex, short axis)
{
    return IMC_ServoOff(cardNumber(cardIndex), axis, 1);
}

int Imc60gApi::setMotionProfile(unsigned int cardIndex, short axis, double velocity,
    double acceleration, double deceleration, double startVelocity)
{
    // V2 caches start velocity but does not send it to this SDK call.
    (void)startVelocity;
    const int rc = IMC_SetAxMvPara(
        cardNumber(cardIndex), axis, velocity, acceleration, deceleration);
    if (rc == 0 && axis >= 0 && static_cast<size_t>(axis) < profileVelocities_.size()) {
        profileVelocities_[static_cast<size_t>(axis)] = velocity;
    }
    return rc;
}

int Imc60gApi::setAxisEndVelocity(
    unsigned int cardIndex, short axis, double endVelocity)
{
    return IMC_SetAxEndVel(cardNumber(cardIndex), axis, endVelocity);
}

int Imc60gApi::startPtp(unsigned int cardIndex, short axis, int target)
{
    return IMC_StartPtpMove(cardNumber(cardIndex), axis, static_cast<double>(target), 0);
}

int Imc60gApi::configureJog(unsigned int cardIndex, short axis)
{
    return IMC_JogPrf(cardNumber(cardIndex), axis);
}

int Imc60gApi::startJogMove(unsigned int cardIndex, short axis, int direction)
{
    if (axis < 0 || static_cast<size_t>(axis) >= profileVelocities_.size()) {
        return ERR_AX_INDEX_OUTRANG;
    }
    const double signedVelocity =
        (direction < 0 ? -1.0 : 1.0) * profileVelocities_[static_cast<size_t>(axis)];
    return IMC_StartJogMove(cardNumber(cardIndex), axis, signedVelocity);
}

int Imc60gApi::stop(unsigned int cardIndex, short axis, int mode)
{
    return IMC_StopMove(cardNumber(cardIndex), axis, static_cast<short>(mode));
}

int Imc60gApi::axisStatus(unsigned int cardIndex, short axis, unsigned int* status)
{
    int nativeStatus = 0;
    const int rc = IMC_GetAxSts(cardNumber(cardIndex), axis, &nativeStatus, 1);
    if (rc == 0 && status) {
        *status = static_cast<unsigned int>(nativeStatus);
    }
    return rc;
}

int Imc60gApi::stopReason(unsigned int cardIndex, short axis, unsigned int* reason)
{
    short nativeReason = 0;
    const int rc = IMC_GetAxStopReason(cardNumber(cardIndex), axis, &nativeReason, 1);
    if (rc == 0 && reason) {
        *reason = static_cast<unsigned short>(nativeReason);
    }
    return rc;
}

int Imc60gApi::plannedPosition(unsigned int cardIndex, short axis, int* position)
{
    return IMC_GetAxPrfPos32(cardNumber(cardIndex), axis, position, 1);
}

int Imc60gApi::encoderPosition(unsigned int cardIndex, short axis, int* position)
{
    return IMC_GetAxEncPos32(cardNumber(cardIndex), axis, position, 1);
}

int Imc60gApi::setCurrentPosition(unsigned int cardIndex, short axis, double position)
{
    return IMC_SetAxCurPos(cardNumber(cardIndex), axis, position);
}

int Imc60gApi::syncPosition(unsigned int cardIndex, short axis)
{
    return IMC_SyncAxPos(cardNumber(cardIndex), axis);
}

int Imc60gApi::setAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
    unsigned short subIndex, unsigned char* data, unsigned int size,
    unsigned int* abortCode)
{
    return IMC_SetEcatAxSdo(
        cardNumber(cardIndex), axis, index, subIndex, data, size, abortCode);
}

int Imc60gApi::getAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
    unsigned short subIndex, unsigned char* data, unsigned int size,
    unsigned int* resultSize, unsigned int* abortCode)
{
    return IMC_GetEcatAxSdo(
        cardNumber(cardIndex), axis, index, subIndex, data, size, resultSize, abortCode);
}

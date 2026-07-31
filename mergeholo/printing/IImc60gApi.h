#pragma once

class IImc60gApi {
public:
    virtual ~IImc60gApi() = default;

    virtual int getCardsNum(unsigned int* count) = 0;
    virtual int openCard(unsigned int cardIndex) = 0;
    virtual int closeCard(unsigned int cardIndex) = 0;
    virtual int scanEthercat(unsigned int cardIndex, short waitSeconds) = 0;
    virtual int initEthercat(unsigned int cardIndex) = 0;
    virtual int startEthercat(unsigned int cardIndex) = 0;
    virtual int stopEthercat(unsigned int cardIndex) = 0;
    virtual int ethercatMasterStatus(unsigned int cardIndex,
        unsigned int* status) = 0;
    virtual int setEmergencyLevel(unsigned int cardIndex, short inverted) = 0;
    virtual int emergencyStatus(unsigned int cardIndex, short* status) = 0;
    virtual int clearAxisStatus(unsigned int cardIndex, short axis) = 0;
    virtual int servoOn(unsigned int cardIndex, short axis) = 0;
    virtual int servoOff(unsigned int cardIndex, short axis) = 0;
    virtual int setMotionProfile(unsigned int cardIndex, short axis, double velocity,
        double acceleration, double deceleration, double startVelocity) = 0;
    virtual int setAxisEndVelocity(unsigned int cardIndex, short axis,
        double endVelocity) = 0;
    virtual int startPtp(unsigned int cardIndex, short axis, int target) = 0;
    virtual int configureJog(unsigned int cardIndex, short axis) = 0;
    virtual int startJogMove(unsigned int cardIndex, short axis, int direction) = 0;
    virtual int stop(unsigned int cardIndex, short axis, int mode) = 0;
    virtual int axisStatus(unsigned int cardIndex, short axis, unsigned int* status) = 0;
    virtual int stopReason(unsigned int cardIndex, short axis, unsigned int* reason) = 0;
    virtual int plannedPosition(unsigned int cardIndex, short axis, int* position) = 0;
    virtual int encoderPosition(unsigned int cardIndex, short axis, int* position) = 0;
    virtual int setCurrentPosition(unsigned int cardIndex, short axis, double position) = 0;
    virtual int syncPosition(unsigned int cardIndex, short axis) = 0;
    virtual int setAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
        unsigned short subIndex, unsigned char* data, unsigned int size,
        unsigned int* abortCode) = 0;
    virtual int getAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
        unsigned short subIndex, unsigned char* data, unsigned int size,
        unsigned int* resultSize, unsigned int* abortCode) = 0;
};

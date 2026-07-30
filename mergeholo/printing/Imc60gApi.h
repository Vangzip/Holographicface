#pragma once

#include "IImc60gApi.h"

#include <array>

class Imc60gApi final : public IImc60gApi {
public:
    Imc60gApi();

    int getCardsNum(unsigned int* count) override;
    int openCard(unsigned int cardIndex) override;
    int closeCard(unsigned int cardIndex) override;
    int scanEthercat(unsigned int cardIndex, short waitSeconds) override;
    int initEthercat(unsigned int cardIndex) override;
    int startEthercat(unsigned int cardIndex) override;
    int stopEthercat(unsigned int cardIndex) override;
    int setEmergencyLevel(unsigned int cardIndex, short inverted) override;
    int clearAxisStatus(unsigned int cardIndex, short axis) override;
    int servoOn(unsigned int cardIndex, short axis) override;
    int servoOff(unsigned int cardIndex, short axis) override;
    int setMotionProfile(unsigned int cardIndex, short axis, double velocity,
        double acceleration, double deceleration, double startVelocity,
        double endVelocity) override;
    int startPtp(unsigned int cardIndex, short axis, int target) override;
    int startJog(unsigned int cardIndex, short axis, int direction) override;
    int stop(unsigned int cardIndex, short axis, int mode) override;
    int axisStatus(unsigned int cardIndex, short axis, unsigned int* status) override;
    int stopReason(unsigned int cardIndex, short axis, unsigned int* reason) override;
    int plannedPosition(unsigned int cardIndex, short axis, int* position) override;
    int encoderPosition(unsigned int cardIndex, short axis, int* position) override;
    int setCurrentPosition(unsigned int cardIndex, short axis, double position) override;
    int syncPosition(unsigned int cardIndex, short axis) override;
    int setAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
        unsigned short subIndex, unsigned char* data, unsigned int size,
        unsigned int* abortCode) override;
    int getAxisSdo(unsigned int cardIndex, short axis, unsigned short index,
        unsigned short subIndex, unsigned char* data, unsigned int size,
        unsigned int* resultSize, unsigned int* abortCode) override;

private:
    std::array<double, 64> profileVelocities_;
};

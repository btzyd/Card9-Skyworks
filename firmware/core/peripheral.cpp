#include "peripheral.h"
#include "protocol.h"
#include "common.h"

void Door::Init()
{
    pinMode(lockMagnetPin, OUTPUT);
    pinMode(lockDetectPin, INPUT);
    digitalWrite(lockMagnetPin, HIGH);//lock door
    digitalWrite(lockDetectPin, HIGH);//enable pull-up

    delay(400);
    lastDectVal = digitalRead(lockDetectPin);
    lastDectTime = millis();
}

void Door::Open()
{
    if (state == DoorLocked) {
        digitalWrite(lockMagnetPin, LOW);
        preparedTimer = millis();
        state = DoorPreparedOpen;
        Serial.println("DoorPreparedOpen");
    }
}

bool Door::detect()
{
    unsigned char ret = digitalRead(lockDetectPin);
    if(millis()-lastDectTime > DetectorSwitchDelay &&
        (ret ^ lastDectVal)){

        lastDectTime = millis();
        lastDectVal = ret;
    }
    return lastDectVal == HIGH;
}

bool Door::UpdateState()
{
    switch (state) {
    case DoorLocked:
        if (detect()) {
            state = DoorOpened;
            Serial.println("DoorOpened Illegally");
            sendEventPacket(DoorDidOpen);
            return true;
        }
        break;
    case DoorPreparedOpen:
        if (millis() - preparedTimer > DoorPreparedOpenTimeOut) {
            digitalWrite(lockMagnetPin, HIGH);
            openedTimer = millis();
            state = DoorOpened;
            Serial.println("DoorOpened");
            sendEventPacket(DoorDidOpen);
        }
        break;
    case DoorOpened:
        if (!detect()) {
            digitalWrite(lockMagnetPin, HIGH);
            state = DoorLocked;
            Serial.println("DoorLocked");
            sendEventPacket(DoorDidClose);
        } else if(millis() - openedTimer > DoorOpenedTimeOut) {
            Serial.println("DoorOpenTimedOut");
            state = DoorOpenTimedOut;
            return true;
        }
        break;
    case DoorOpenTimedOut:
        if (!detect()) {
            digitalWrite(lockMagnetPin, HIGH);
            state = DoorLocked;
            Serial.println("DoorLocked");
            sendEventPacket(DoorDidClose);
        }
        break;
    }
    return false;
}

void Alarm::Init()
{
    pinMode(alarmPin, OUTPUT);
    digitalWrite(alarmPin, LOW);
}

void Alarm::setAlarm(bool _on)
{
    if(on != _on){
        on = _on;
        digitalWrite(alarmPin, _on ? HIGH : LOW);
        sendEventPacket(_on ? AlarmDidOn : AlarmDidOff);
    }
}

void Button::Init()
{
    pinMode(buttonPin, INPUT);
    digitalWrite(buttonPin, HIGH);//enable pull-up
    lastState = digitalRead(buttonPin);
    latestAction = ActionNone;
}

void Button::Check()
{
    unsigned long now = millis();
    uint8_t val = digitalRead(buttonPin);
    if((val^lastState)){
        lastState = val;
        if(val == LOW){ //key down
            pressTime = now;
            Serial.println("Button down");
        }else{ //key up
            if(now - pressTime > ValidPressThreshold
                && now - pressTime < LongPressThreshold){

                latestAction = ActionPressed;
            }
            Serial.println("Button up");
        }
    }else{
        if(val == LOW && now - pressTime > LongPressThreshold){
            latestAction = ActionLongPressed;
            pressTime = now;
        }
    }
}



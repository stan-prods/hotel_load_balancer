#define statesAmount 5
#include <LinkedList.h>

class Status;

struct Flags {
    bool isMainsPresent;
    bool isGeneratorWorking;

    bool isValveOpen;
    bool isValveOnBattery;
    bool isStarterRunning;
};

enum event {
    startCommand,
    stopCommand,
    flagsStateRequest,
    applyFlagsRequest
};

class EventReciever {
    public:
        virtual void fireEvent (event ev, Flags& f, EventReciever* ER) {this->onIncomingEvent(ev, f, ER);};

    protected:
        virtual void onIncomingEvent (event, Flags&, EventReciever*) =0;

};

class Observable {
    protected:
        LinkedList<EventReciever*> recievers;

        void bradcastEvent (event ev, Flags& f, EventReciever* er) {
            for (int i = 0; i < recievers.size(); i++) {
                recievers.get(i)->fireEvent(ev, f, er);
            }
        };

    public:
        subscibe(EventReciever* er) {
            recievers.add(er);
        }
};


class Status : public EventReciever, public Observable {
    private:
        Flags flags;

        Flags& copyFlags () {
            Flags copy;
            copy = this->flags;

            return copy;
        };

    public:
        Flags& getFlags () {
            return this->copyFlags();
        };

        void updateFlags () {
            Flags requestedFlags = this->copyFlags();

            this->bradcastEvent(flagsStateRequest, flags, this);

            this->flags = requestedFlags;
        };

        void applyFlags () {
            this->bradcastEvent(applyFlagsRequest, this->copyFlags(), this);
        };


    protected:
        virtual void onIncomingEvent (event, Flags&, EventReciever*);
};

class State : public EventReciever {
    public:
        virtual State* leaving() {return this;};
        virtual State* entering() {return this;};

    protected:
        virtual void onIncomingEvent (event, Flags&, EventReciever*);

};

class Idle : public State {

};

class Starting : public State {
    public:
        virtual State* entering () {
            //start powering valve with battery

        };

        virtual State* leaving () {
            //stop starter, stop powering valve from battery
        };
    private:
        void startGenerator() {
            if (attempts > allowedAttempts - 1) {
                return;
            }
            //open gas valve
            powerValveFromBattery();
            enableValve();

            //if first attempt, let gas flow
            if (attempts == 0 && gasValveOpenOnStartTimestampSecs == 0) {
                gasValveOpenOnStartTimestampSecs = secs;
            }

            if (secs - gasValveOpenOnStartTimestampSecs > gasValveOpenDelaySecs) {
                gasValveOpenOnStartTimestampSecs = 0;
                runStarter();
            }

        }

        void runStarter() {
            if (isGeneratorWorking) {
                return;
            }
            Serial.print(", Run Starter");
            if (!isStarterRunning && secs - starterPauseStartedTimestampSecs > starterPauseDelaySecs) {
                digitalWrite(starterRelayPin, enable);
                starterEnabledTimestampSecs = secs;
                isStarterRunning = true;
            }
        }

        void stopStarter() {
            Serial.print(", Stop Starter");
            digitalWrite(starterRelayPin, disable);
            starterEnabledTimestampSecs = 0;
            gasValveOpenOnStartTimestampSecs = 0;
            isStarterRunning = false;
        }

        void checkStarter() {
            if (!isStarterRunning) {
                return;
            }

            if (isMainsPresent) {
                stopStarter();
                attempts = 0;

                enableValve(false);
            }

            if (isGeneratorWorking) {
                Serial.print(", Generator started");
                stopStarter();
                attempts = 0;

                powerValveFromBattery(false);
            } else if (secs - starterEnabledTimestampSecs >= starterActiveDelaySecs) {
                Serial.print(", Generator start failed");
                stopStarter();
                attempts++;

                enableValve(false);

                starterPauseStartedTimestampSecs = secs;
            }
        }

};

class Running : public State {
    void stopGenerator() {
        Serial.print(", Stop generator");
        if (!generatorCooldownStartedTimestampSecs) {
            generatorCooldownStartedTimestampSecs = secs;
        }

        if (secs - generatorCooldownStartedTimestampSecs >= generatorCooldownDelaySecs) {
            digitalWrite(gasValveRelayPin, disable);
            isValveOpen = false;
            generatorCooldownStartedTimestampSecs = 0;
        }
    }

};
class Stoping : public State {};
class IdleOnError : public State {};

class Statefull : public EventReciever {
    private:
        State states[statesAmount];

    protected:
        enum stateName {};
        int state;

        State* setState (stateName name) {
            this->getState()->leaving();

            this->state = name;
            return this->getState()->entering();
        };
        State* getState () {
            return &this->states[this->state];
        };
};

class Generator : public Statefull {
    public:
        bool start () {
            this->getState()->fireEvent(startCommand, status.getFlags(), this);
        };
        bool stop () {
            this->getState()->fireEvent(stopCommand, status.getFlags(), this);
        };
        Status& getStatus () {return this->status;};
    protected:
        enum stateName {idle, idleOnError, starting, stoping, running};
        virtual void onIncomingEvent (event);

    private:
        Status status;
        State states[statesAmount] {
            Idle(),
            IdleOnError(),
            Starting(),
            Stoping(),
            Running()
        };
};


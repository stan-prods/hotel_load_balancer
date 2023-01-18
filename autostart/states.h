#define statesAmount 5

struct Flags {
    bool isMainsPresent = false;
    bool isGeneratorWorking = false;
    bool isValveOpen = false;
    bool isValveOnBattery = false;
    bool isStarterRunning = false;
};

enum event {startCommand, stopCommand};

class EventReciever {
    public:
        virtual void fireEvent (event ev, Flags& f, EventReciever* ER) {this->onIncomingEvent(ev, f, ER);};

    protected:
        virtual void onIncomingEvent (event, Flags&, EventReciever*) =0;

};
class State : public EventReciever {
    public:
        virtual State* leaving() {return this;};
        virtual State* entering() {return this;};

    protected:
        virtual void onIncomingEvent (event, Flags&, EventReciever*);

};

class Statefull : public EventReciever {
    private:
        State states[statesAmount];

    protected:
        Flags flags;
        enum stateName {};
        uint8_t state;

        State* setState (stateName name) {
            this->getState()->leaving();

            this->state = name;
            return this->getState()->entering();
        };
        State* getState () {
            return &this->states[this->state];
        };
};

class Idle : public State {};
class Starting : public State {};
class Running : public State {};
class Stoping : public State {};
class IdleOnError : public State {};

class Generator : public Statefull {
    public:
        bool start () {
            this->getState()->fireEvent(startCommand, flags, this);
        };
        bool stop () {
            this->getState()->fireEvent(stopCommand, flags, this);
        };
    protected:
        enum stateName {idle, idleOnError, starting, stoping, running};
        virtual void onIncomingEvent (event);

    private:
        State states[statesAmount] {
            Idle(),
            IdleOnError(),
            Starting(),
            Stoping(),
            Running()
        };
};


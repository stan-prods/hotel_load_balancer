#define statesAmount 5

struct Flags {
    bool isMainsPresent = false;
    bool isGeneratorWorking = false;
    bool isValveOpen = false;
    bool isValveOnBattery = false;
    bool isStarterRunning = false;
};

enum event {startCommand, stopCommand};

class StateCallback {
    public:
        virtual void fireEvent (event ev) {this->onStateEvent(ev);};
        virtual bool applyFlags() =0;

    protected:
        virtual void onStateEvent (event) =0;

};
class State {
    public:
        virtual void notify(event ev, Flags& f, StateCallback* SC) {this->onEvent(ev, f, SC);};
        virtual State* leaving() {return this;};
        virtual State* entering() {return this;};

    private:
        virtual void onEvent(event, Flags&, StateCallback*);
};

class Statefull : public StateCallback {
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
            this->getState()->notify(startCommand, flags, this);
        };
        bool stop () {
            this->getState()->notify(stopCommand, flags, this);
        };
    protected:
        enum stateName {idle, idleOnError, starting, stoping, running};
        virtual void onStateEvent (event);

    private:
        State states[statesAmount] {
            Idle(),
            IdleOnError(),
            Starting(),
            Stoping(),
            Running()
        };
};


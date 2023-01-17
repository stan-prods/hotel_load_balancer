#define statesAmount 5

struct Flags {};

class State {
    public:
        virtual void notify(String*, Flags*);
        virtual State* leaving() {return this;};
        virtual State* entering() {return this;};
};

class Statefull {
    public:
        virtual void onStateEvent (String*) =0;

    protected:
        enum stateName {};
        uint8_t state;
        State *states[statesAmount];

        State* setState (stateName name) {
            this->getState()->leaving();

            this->state = name;
            return this->getState()->entering();
        };
        State* getState () {
            return this->states[this->state];
        };
};

class Idle : public State {
    public:
        virtual void notify(String*, Flags*);
};

class Starting : public State {
    public:
        virtual void notify(String*, Flags*);
};

class Running : public State {
    public:
        virtual void notify(String*, Flags*);
};

class Stoping : public State {
    public:
        virtual void notify(String*, Flags*);
};

class IdleOnError : public State {
    public:
        virtual void notify(String*, Flags*);
};

class Generator : public Statefull {
    public:
        virtual void onStateEvent (String*);

    protected:
        enum stateName {idle, idleOnError, starting, stoping, running};
        State states[statesAmount] {
            Idle(),
            IdleOnError(),
            Starting(),
            Stoping(),
            Running()
        };
};


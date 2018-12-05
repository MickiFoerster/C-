#include <iostream>

class LightSwitchTransition;

struct State {
  virtual void on(LightSwitchTransition* lhs) {
    std::cout << "Light is already on\n";
  }
  virtual void off(LightSwitchTransition* lhs) {
    std::cout << "Light is already on\n";
  }
};

struct OnState : State {
  OnState() {
    std::cout << "Light is ON\n";
  }

  void off(LightSwitchTransition *lhs) override ;
};

struct OffState : State {
  OffState() {
    std::cout << "Light is OFF\n";
  }
  void on(LightSwitchTransition *lhs) override ;
};

class LightSwitchTransition {
  State *state;
public:
  LightSwitchTransition() {
    state = new OffState();
  }

  void set_state(State* state) {
    this->state = state;
  }

  void on() {
    state->on(this);
  }
  void off() {
    state->off(this);
  }
};

void OffState::on(LightSwitchTransition *lhs) {
  std::cout << "Switching light on ...\n";
  lhs->set_state(new OnState());
  delete this;
}

void OnState::off(LightSwitchTransition *lhs) {
  std::cout << "Switching light off ...\n";
  lhs->set_state(new OffState());
  delete this;
}

int main(int argc, char* argv[]) {
  LightSwitchTransition ls;
  ls.on();
  ls.off();
  ls.off();
  return 0;
}

#ifndef LEAKY_INTEGRATOR_H
#define LEAKY_INTEGRATOR_H

#if __cplusplus
extern "C" {
#endif

class LeakyIntegrator {

public:

  static constexpr double kDefaultP = 0.5;

  LeakyIntegrator( void );
  virtual ~LeakyIntegrator( void );

  void reset( void ) {
    state_ = 0.0;
  }

  void setP(double p) {
    p_ = p;
    q_ = 1.0 - p;
  }

  double getP( void ) {
    return p_;
  }

  double read() {
    return state_;
  }

  void write(double value) {
    state_ = q_ * state_ + p_ * value;
  }

private:

  double state_;
  double q_;
  double p_;
};

#if __cplusplus
}
#endif
#endif

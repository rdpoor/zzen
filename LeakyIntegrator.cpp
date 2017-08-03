#include "LeakyIntegrator.h"

LeakyIntegrator::LeakyIntegrator( void ) {
  this->reset();
  this->setP(LeakyIntegrator::kDefaultP);
}

LeakyIntegrator::~LeakyIntegrator( void ) {
}

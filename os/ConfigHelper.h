/*
 * ConfigHelper.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef CONFIGHELPER_H_
#define CONFIGHELPER_H_

#include "RoundRobinPolicy.h"

class Selector;

template<class Selector>
class PolicySwitch: public RoundRobinPolicy {

};

#endif /* CONFIGHELPER_H_ */

/*
 * OsTestPlugin.h
 *
 *      Author: tamas.seller
 */

#ifndef OSTESTPLUGIN_H_
#define OSTESTPLUGIN_H_

#include "CommonTestUtils.h"

class OsTestPlugin: public pet::TestPlugin
{
    inline virtual void beforeTest() {
        StackPool::clear();
    }

    inline virtual void afterTest() {
        CommonTestUtils::registerIrq(nullptr);
    }

public:
    inline virtual ~OsTestPlugin() {}
};

#endif /* OSTESTPLUGIN_H_ */

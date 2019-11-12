/**
 * @file   coroutine_test.cpp
 * @author Nat Goodspeed
 * @date   2009-04-22
 * @brief  Test for coroutine.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#define BOOST_RESULT_OF_USE_TR1 1
#include <boost/bind.hpp>
#include <boost/range.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "linden_common.h"

#include <iostream>
#include <string>

#include "../test/lltut.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llevents.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "../test/debug.h"
#include "../test/sync.h"

using namespace llcoro;

/*****************************************************************************
*   Test helpers
*****************************************************************************/
/// Simulate an event API whose response is immediate: sent on receipt of the
/// initial request, rather than after some delay. This is the case that
/// distinguishes postAndSuspend() from calling post(), then calling
/// suspendUntilEventOn().
class ImmediateAPI
{
public:
    ImmediateAPI(Sync& sync):
        mPump("immediate", true),
        mSync(sync)
    {
        mPump.listen("API", boost::bind(&ImmediateAPI::operator(), this, _1));
    }

    LLEventPump& getPump() { return mPump; }

    // Invoke this with an LLSD map containing:
    // ["value"]: Integer value. We will reply with ["value"] + 1.
    // ["reply"]: Name of LLEventPump on which to send response.
    bool operator()(const LLSD& event) const
    {
        mSync.bump();
        LLSD::Integer value(event["value"]);
        LLEventPumps::instance().obtain(event["reply"]).post(value + 1);
        return false;
    }

private:
    LLEventStream mPump;
    Sync& mSync;
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct test_data
        {
        Sync mSync;
        ImmediateAPI immediateAPI{mSync};
    std::string replyName, errorName, threw, stringdata;
    LLSD result, errordata;
    int which;

        void explicit_wait(boost::shared_ptr<LLCoros::Promise<std::string>>& cbp);
        void waitForEventOn1();
        void coroPump();
        void postAndWait1();
        void coroPumpPost();
    };
    typedef test_group<test_data> coroutine_group;
    typedef coroutine_group::object object;
    coroutine_group coroutinegrp("coroutine");

    void test_data::explicit_wait(boost::shared_ptr<LLCoros::Promise<std::string>>& cbp)
    {
        BEGIN
        {
            mSync.bump();
            // The point of this test is to verify / illustrate suspending a
            // coroutine for something other than an LLEventPump. In other
            // words, this shows how to adapt to any async operation that
            // provides a callback-style notification (and prove that it
            // works).

            // Perhaps we would send a request to a remote server and arrange
            // for cbp->set_value() to be called on response.
            // For test purposes, instead of handing 'callback' (or an
            // adapter) off to some I/O subsystem, we'll just pass it back to
            // our caller.
            cbp = boost::make_shared<LLCoros::Promise<std::string>>();
            LLCoros::Future<std::string> future = LLCoros::getFuture(*cbp);

            // calling get() on the future causes us to suspend
            debug("about to suspend");
            stringdata = future.get();
            mSync.bump();
            ensure_equals("Got it", stringdata, "received");
        }
        END
    }

    template<> template<>
    void object::test<1>()
    {
        set_test_name("explicit_wait");
        DEBUG;

        // Construct the coroutine instance that will run explicit_wait.
        boost::shared_ptr<LLCoros::Promise<std::string>> respond;
        LLCoros::instance().launch("test<1>",
                                   [this, &respond](){ explicit_wait(respond); });
        mSync.bump();
        // When the coroutine waits for the future, it returns here.
        debug("about to respond");
        // Now we're the I/O subsystem delivering a result. This should make
        // the coroutine ready.
        respond->set_value("received");
        // but give it a chance to wake up
        mSync.yield();
        // ensure the coroutine ran and woke up again with the intended result
        ensure_equals(stringdata, "received");
    }

    void test_data::waitForEventOn1()
    {
        BEGIN
        {
            mSync.bump();
            result = suspendUntilEventOn("source");
            mSync.bump();
        }
        END
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("waitForEventOn1");
        DEBUG;
        LLCoros::instance().launch("test<2>", [this](){ waitForEventOn1(); });
        mSync.bump();
        debug("about to send");
        LLEventPumps::instance().obtain("source").post("received");
        // give waitForEventOn1() a chance to run
        mSync.yield();
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    void test_data::coroPump()
    {
        BEGIN
        {
            mSync.bump();
            LLCoroEventPump waiter;
            replyName = waiter.getName();
            result = waiter.suspend();
            mSync.bump();
        }
        END
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("coroPump");
        DEBUG;
        LLCoros::instance().launch("test<3>", [this](){ coroPump(); });
        mSync.bump();
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        // give coroPump() a chance to run
        mSync.yield();
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    void coroPumps()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            replyName = waiter.getName0();
            errorName = waiter.getName1();
            LLEventWithID pair(waiter.suspend());
            result = pair.first;
            which  = pair.second;
        }
        END
    }

    template<> template<>
    void object::test<7>()
    {
        clear();
        set_test_name("coroPumps reply");
        DEBUG;
        LLCoros::instance().launch("test<7>", coroPumps);
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_equals(result.asString(), "received");
        ensure_equals("which pump", which, 0);
    }

    template<> template<>
    void object::test<8>()
    {
        clear();
        set_test_name("coroPumps error");
        DEBUG;
        LLCoros::instance().launch("test<8>", coroPumps);
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure_equals(result.asString(), "badness");
        ensure_equals("which pump", which, 1);
    }

    void coroPumpsNoEx()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            replyName = waiter.getName0();
            errorName = waiter.getName1();
            result = waiter.suspendWithException();
        }
        END
    }

    template<> template<>
    void object::test<9>()
    {
        clear();
        set_test_name("coroPumpsNoEx");
        DEBUG;
        LLCoros::instance().launch("test<9>", coroPumpsNoEx);
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    void coroPumpsEx()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            replyName = waiter.getName0();
            errorName = waiter.getName1();
            try
            {
                result = waiter.suspendWithException();
                debug("no exception");
            }
            catch (const LLErrorEvent& e)
            {
                debug(STRINGIZE("exception " << e.what()));
                errordata = e.getData();
            }
        }
        END
    }

    template<> template<>
    void object::test<10>()
    {
        clear();
        set_test_name("coroPumpsEx");
        DEBUG;
        LLCoros::instance().launch("test<10>", coroPumpsEx);
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure("no result", result.isUndefined());
        ensure_equals("got error", errordata.asString(), "badness");
    }

    void coroPumpsNoLog()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            replyName = waiter.getName0();
            errorName = waiter.getName1();
            result = waiter.suspendWithLog();
        }
        END
    }

    template<> template<>
    void object::test<11>()
    {
        clear();
        set_test_name("coroPumpsNoLog");
        DEBUG;
        LLCoros::instance().launch("test<11>", coroPumpsNoLog);
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    void coroPumpsLog()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            replyName = waiter.getName0();
            errorName = waiter.getName1();
            WrapLLErrs capture;
            try
            {
                result = waiter.suspendWithLog();
                debug("no exception");
            }
            catch (const WrapLLErrs::FatalException& e)
            {
                debug(STRINGIZE("exception " << e.what()));
                threw = e.what();
            }
        }
        END
    }

    template<> template<>
    void object::test<12>()
    {
        clear();
        set_test_name("coroPumpsLog");
        DEBUG;
        LLCoros::instance().launch("test<12>", coroPumpsLog);
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure("no result", result.isUndefined());
        ensure_contains("got error", threw, "badness");
    }

    void postAndWait1()
    {
        BEGIN
        {
            mSync.bump();
            result = postAndSuspend(LLSDMap("value", 17),       // request event
                                 immediateAPI.getPump(),     // requestPump
                                 "reply1",                   // replyPump
                                 "reply");                   // request["reply"] = name
            mSync.bump();
        }
        END
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("postAndWait1");
        DEBUG;
        LLCoros::instance().launch("test<4>", [this](){ postAndWait1(); });
        ensure_equals(result.asInteger(), 18);
    }

    void test_data::coroPumpPost()
    {
        BEGIN
        {
            mSync.bump();
            LLCoroEventPump waiter;
            result = waiter.postAndSuspend(LLSDMap("value", 17),
                                        immediateAPI.getPump(), "reply");
            mSync.bump();
        }
        END
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("coroPumpPost");
        DEBUG;
        LLCoros::instance().launch("test<5>", [this](){ coroPumpPost(); });
        ensure_equals(result.asInteger(), 18);
    }

    void coroPumpsPost()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            LLEventWithID pair(waiter.postAndSuspend(LLSDMap("value", 23),
                                                  immediateAPI.getPump(), "reply", "error"));
            result = pair.first;
            which  = pair.second;
        }
        END
    }

    template<> template<>
    void object::test<17>()
    {
        clear();
        set_test_name("coroPumpsPost reply");
        DEBUG;
        LLCoros::instance().launch("test<17>", coroPumpsPost);
        ensure_equals(result.asInteger(), 24);
        ensure_equals("which pump", which, 0);
    }

    void coroPumpsPost_1()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            LLEventWithID pair(
                waiter.postAndSuspend(LLSDMap("value", 23)("fail", LLSD()),
                                   immediateAPI.getPump(), "reply", "error"));
            result = pair.first;
            which  = pair.second;
        }
        END
    }

    template<> template<>
    void object::test<18>()
    {
        clear();
        set_test_name("coroPumpsPost error");
        DEBUG;
        LLCoros::instance().launch("test<18>", coroPumpsPost_1);
        ensure_equals(result.asInteger(), 24);
        ensure_equals("which pump", which, 1);
    }

    void coroPumpsPostNoEx()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            result = waiter.postAndSuspendWithException(LLSDMap("value", 8),
                                                     immediateAPI.getPump(), "reply", "error");
        }
        END
    }

    template<> template<>
    void object::test<19>()
    {
        clear();
        set_test_name("coroPumpsPostNoEx");
        DEBUG;
        LLCoros::instance().launch("test<19>", coroPumpsPostNoEx);
        ensure_equals(result.asInteger(), 9);
    }

    void coroPumpsPostEx()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            try
            {
                result = waiter.postAndSuspendWithException(
                    LLSDMap("value", 9)("fail", LLSD()),
                    immediateAPI.getPump(), "reply", "error");
                debug("no exception");
            }
            catch (const LLErrorEvent& e)
            {
                debug(STRINGIZE("exception " << e.what()));
                errordata = e.getData();
            }
        }
        END
    }

    template<> template<>
    void object::test<20>()
    {
        clear();
        set_test_name("coroPumpsPostEx");
        DEBUG;
        LLCoros::instance().launch("test<20>", coroPumpsPostEx);
        ensure("no result", result.isUndefined());
        ensure_equals("got error", errordata.asInteger(), 10);
    }

    void coroPumpsPostNoLog()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            result = waiter.postAndSuspendWithLog(LLSDMap("value", 30),
                                               immediateAPI.getPump(), "reply", "error");
        }
        END
    }

    template<> template<>
    void object::test<21>()
    {
        clear();
        set_test_name("coroPumpsPostNoLog");
        DEBUG;
        LLCoros::instance().launch("test<21>", coroPumpsPostNoLog);
        ensure_equals(result.asInteger(), 31);
    }

    void coroPumpsPostLog()
    {
        BEGIN
        {
            LLCoroEventPumps waiter;
            WrapLLErrs capture;
            try
            {
                result = waiter.postAndSuspendWithLog(
                    LLSDMap("value", 31)("fail", LLSD()),
                    immediateAPI.getPump(), "reply", "error");
                debug("no exception");
            }
            catch (const WrapLLErrs::FatalException& e)
            {
                debug(STRINGIZE("exception " << e.what()));
                threw = e.what();
            }
        }
        END
    }

    template<> template<>
    void object::test<22>()
    {
        clear();
        set_test_name("coroPumpsPostLog");
        DEBUG;
        LLCoros::instance().launch("test<22>", coroPumpsPostLog);
        ensure("no result", result.isUndefined());
        ensure_contains("got error", threw, "32");
    }
}

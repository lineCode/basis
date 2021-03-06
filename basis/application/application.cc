#include "basis/application/application.hpp" // IWYU pragma: associated

#include <base/strings/stringprintf.h>
#include <base/trace_event/trace_event.h>
#include <base/message_loop/message_loop_current.h>

namespace application {

ApplicationObserver::ApplicationObserver() = default;

ApplicationObserver::~ApplicationObserver() = default;

#define STATE_STRING(state)                                             \
  base::StringPrintf("%s (%d)", \
    application::GetApplicationStateString(state), \
    static_cast<int>(state))

Application::Application()
  : observers_(new base::ObserverListThreadSafe<ApplicationObserver>())
  , app_loaded_(base::WaitableEvent::ResetPolicy::MANUAL,
                base::WaitableEvent::InitialState::NOT_SIGNALED)
{
  DETACH_FROM_SEQUENCE(sequence_checker_);

  // init threading before |Application| constructor
  DCHECK(base::MessageLoopCurrent::Get());
}

Application::~Application()
{
  DCHECK(application_state_ == application::kApplicationStateStopped);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void
  Application::notifyStateChange(
    const application::ApplicationState& state)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  observers_->Notify(FROM_HERE
    , &ApplicationObserver::onStateChange
    , state);
}

void
  Application::notifyFocusChange(
    const bool has_focus)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  observers_->Notify(FROM_HERE
    , &ApplicationObserver::onFocusChange
    , has_focus);
}

void
  Application::setApplicationState(
    application::ApplicationState state)
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  TRACE_EVENT1("headless"
    , "SetApplicationState"
    , "state"
    , STATE_STRING(state));

  if (application_state_ == state) {
    DLOG(WARNING) << __FUNCTION__ << ": Attempt to re-enter "
                  << STATE_STRING(application_state_);
    return;
  }

  // Audit that the transitions are correct.
  if (DLOG_IS_ON(FATAL))
  {
    switch (application_state_) // previous state
    {
      case application::kApplicationStatePaused:
        DCHECK(state == application::kApplicationStateSuspended ||
               state == application::kApplicationStateStarted)
            << ": application_state_=" << STATE_STRING(application_state_)
            << ", state=" << STATE_STRING(state);

        break;
      case application::kApplicationStatePreloading:
        DCHECK(state == application::kApplicationStateSuspended ||
               state == application::kApplicationStateStarted)
            << ": application_state_=" << STATE_STRING(application_state_)
            << ", state=" << STATE_STRING(state);
        break;
      case application::kApplicationStateStarted:
        DCHECK(state == application::kApplicationStatePaused)
            << ": application_state_=" << STATE_STRING(application_state_)
            << ", state=" << STATE_STRING(state);
        break;
      case application::kApplicationStateStopped:
        DCHECK(state == application::kApplicationStatePreloading ||
               state == application::kApplicationStateStarted)
            << ": application_state_=" << STATE_STRING(application_state_)
            << ", state=" << STATE_STRING(state);
        break;
      case application::kApplicationStateSuspended:
        DCHECK(state == application::kApplicationStatePaused ||
               state == application::kApplicationStateStopped)
            << ": application_state_=" << STATE_STRING(application_state_)
            << ", state=" << STATE_STRING(state);
        break;
      default:
        NOTREACHED() << ": application_state_="
                     << STATE_STRING(application_state_)
                     << ", state=" << STATE_STRING(state);

        break;
    } // switch
  } // DLOG_IS_ON(FATAL)

  const bool old_has_focus = HasFocus(application_state_);

  DLOG(INFO) << __FUNCTION__ << ": " << STATE_STRING(application_state_)
             << " -> " << STATE_STRING(state);
  application_state_ = state;
  DCHECK(application_state_ != application::kApplicationStateTotal);

  notifyStateChange(application_state_);

  const bool has_focus = HasFocus(application_state_);
  const bool focus_changed = has_focus != old_has_focus;

  if (focus_changed) {
    notifyFocusChange(has_focus);
  }
}

bool
  Application::HasFocus(
    application::ApplicationState state)
{
  switch (state) {
    case application::kApplicationStateStarted:
      return true;
    case application::kApplicationStatePreloading:
    case application::kApplicationStatePaused:
    case application::kApplicationStateSuspended:
    case application::kApplicationStateStopped:
      return false;
    default:
      NOTREACHED()
        << "Invalid Application State: "
        << STATE_STRING(state);
      return false;
  }
}

void
  Application::initialize()
{
  TRACE_EVENT0("headless", "Application::initialize()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(application_state_ == application::kApplicationStatePreloading);
}

void
  Application::teardown()
{
  TRACE_EVENT0("headless", "Application::teardown()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  setApplicationState(application::kApplicationStateStopped);
}

void
  Application::pause()
{
  TRACE_EVENT0("headless", "Application::pause()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  NOTREACHED() << "application does not support the pause().";
}

void
  Application::start()
{
  TRACE_EVENT0("headless", "Application::start()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // resources must be loaded
  DCHECK(application_state_ == application::kApplicationStatePreloading);

  setApplicationState(application::kApplicationStateStarted);
}

void
  Application::addObserver(
    ApplicationObserver* observer)
{
  TRACE_EVENT0("headless", "Application::addObserver()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(observer);

  observers_->AddObserver(observer);
}

void
  Application::removeObserver(
    ApplicationObserver* observer)
{
  TRACE_EVENT0("headless", "Application::removeObserver()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(observer);

  observers_->RemoveObserver(observer);
}

void
  Application::suspend()
{
  TRACE_EVENT0("headless", "Application::suspend()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // need to pause before resourse unloading
  if(application_state_ != application::kApplicationStatePaused) {
    setApplicationState(application::kApplicationStatePaused);
  }

  // resourse unloading here
  setApplicationState(application::kApplicationStateSuspended);

  app_loaded_.Reset();
}

bool
  Application::waitForLoad(
    const base::TimeDelta& timeout)
{
  TRACE_EVENT0("headless", "Application::waitForLoad()");

  return app_loaded_.TimedWait(timeout);
}

void
  Application::signalOnLoad()
{
  TRACE_EVENT0("headless", "Application::signalOnLoad()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  app_loaded_.Signal();
}

void
  Application::resume()
{
  TRACE_EVENT0("headless", "Application::resume()");

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  NOTREACHED() << "application does not support the resume().";
}

ApplicationState
  Application::getApplicationState()
    const
    noexcept
{
  DCHECK(application_state_ != application::kApplicationStateTotal);
  return application_state_;
}

} // namespace application

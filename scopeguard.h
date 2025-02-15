#ifndef SCOPE_GUARD_H
#define SCOPE_GUARD_H

#include <functional>

#define AtScopeExit(...) ScopeGuard _scope_exit_action_##__LINE__(__VA_ARGS__); \
_scope_exit_action_##__LINE__.used()

class ScopeGuard {
public:
	typedef std::function<void()> ScopeExitFunc;

	ScopeGuard(ScopeExitFunc func)
		: _func(func) {
	}

	~ScopeGuard() {
		if (_func) {
			_func();
		}
	}

	void used() {
	}

	void dismiss() {
		ScopeExitFunc f;
		_func.swap(f);
	}

private:
	ScopeExitFunc _func;
};

#endif // !SCOPE_GUARD_H

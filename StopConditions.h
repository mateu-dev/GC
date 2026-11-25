#include <chrono>

class RunCondition {
public:
	virtual bool ShouldContinue() const = 0;

	virtual ~RunCondition() = default;
	virtual void Reset() {};

	virtual void IterationCompleted() {};
	virtual void ImprovementFound() {};
};

class TimeLimitCondition : public RunCondition {
	std::chrono::steady_clock::duration limit;
	std::chrono::steady_clock::time_point start_time;
	std::chrono::steady_clock::time_point end_time;

public:
	TimeLimitCondition(std::chrono::steady_clock::duration limit) : limit(limit) {
		Reset();
	}

	bool ShouldContinue() const override {
		return std::chrono::steady_clock::now() < end_time;
	};

	void Reset() override {
		start_time = std::chrono::steady_clock::now();
		end_time = start_time + limit;
	}
};

class IterationLimitCondition :public RunCondition {
	int max_iterations;
	int current_iteration{ 0 };

public:
	IterationLimitCondition(int max_iterations) : max_iterations(max_iterations) {
		Reset();
	}

	bool ShouldContinue() const override {
		return current_iteration < max_iterations;
	}

	void Reset() override {
		current_iteration = 0;
	}

	void IterationCompleted() override {
		current_iteration++;
	}

	void ImprovementFound() override {
		current_iteration = 0;
	}
};
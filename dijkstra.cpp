#include <thread>
#include <random>
#include <cmath>
#include <ctime>
#include <list>
#include <iostream>
#include <sstream>
#include <unistd.h>

std::mt19937 rand_gen(std::random_device{}());
unsigned int NUM_PROCS;
int PIPE_READ, PIPE_WRITE;

class Process
{
public:
	Process(unsigned int identification, unsigned int value, const Process* previous):
		id(identification),
		val(value),
		prev(previous),
		thread(&Process::run, this)
	{}
	
	Process(Process&& o) = default;

	virtual ~Process() = default;

	virtual bool isPrivileged() const
	{
		return val != prev->val;
	}

	unsigned int getId() const
	{
		return id;
	}

protected:
	virtual bool perform() {
		if(val != prev->val) {
			val = prev->val;
			return true;
		}
		return false;
	}

	void run() {
		for(;;) {
			// Wait a random ammount of time to perform
			double t = std::uniform_real_distribution<>{}(rand_gen);
			t = log(1.0 - t) * -5.0;
			struct timespec req, rem;
			req.tv_sec = t;
			req.tv_nsec = (t - req.tv_sec) * 1000000000;

			while(nanosleep(&req, &rem) < 0) {
				rem = req;
			}

			// Do perform
			if(perform())
				write(PIPE_WRITE, &id, sizeof(id));
		}
	}

	unsigned int id;
	unsigned int val;
	const Process* prev;

	std::thread thread;

	friend class SpecialProcess;
};

std::ostream& operator<< (std::ostream& stream, const Process& p)
{
	if(p.isPrivileged()) {
		stream << "p̲";
		std::stringstream fmt;
		fmt << p.getId();
		auto sid = fmt.str();
		for(char c: sid) {
			stream << c << "\u0332" ;
		}
	} else {
		stream << 'p' << p.getId();
	}
	return stream;
}

class SpecialProcess: public Process
{
public:
	SpecialProcess(unsigned int value):
		Process(0, value, nullptr)
	{}

	void setPrevious(Process* previous)
	{
		prev = previous;
	}

	virtual bool isPrivileged() const
	{
		if(!prev)
			return false;
		return val == prev->val;
	}

protected:
	virtual bool perform() {
		if(prev && (val == prev->val)) {
			val = (val + 1) % NUM_PROCS;
			return true;
		}
		return false;
	}
};

int main(int argc, char *argv[])
{
	if(argc < 2) {
		std::cerr << "Error: missing number of processes.\n";
		return 1;
	}

	NUM_PROCS = atoi(argv[1]);
	if(NUM_PROCS < 3) {
		std::cerr << "Error: there must be at least 3 processes for the algorithm to make sense.\n";
		return 2;
	}

	// Feedback pipe, so this thread can know what the process are doing.
	{
		int pipefds[2];
		pipe(pipefds);
		PIPE_READ = pipefds[0];
		PIPE_WRITE = pipefds[1];
	}

	// Building all process so all will be privileged.
	SpecialProcess p0(0);
	std::list<Process> procs;

	procs.emplace_back(1, 1, &p0);
	{
		int i;
		for(i = 2; i < NUM_PROCS - 1; ++i) {
			procs.emplace_back(i, i, &procs.back());
		}
		procs.emplace_back(i, 0, &procs.back());
	}
	p0.setPrevious(&procs.back());

	// Print initial state
	std::cout << p0;
	for(const Process &p: procs)
		std::cout << ',' << p;
	std::cout << '\n' << std::endl;

	// The algorithm is running by now... so just wait for action
	// from the "processes".
	for(;;)
	{
		unsigned int pid;
		read(PIPE_READ, &pid, sizeof(pid));

		// Print new state, highlighting the process that acted
		std::cout << p0;
		if(pid == 0)
			std::cout << "\u030C";
		for(const Process &p: procs) {
			std::cout << ',' << p;
			if(p.getId() == pid)
				std::cout << "\u030C";

		}
		std::cout << '\n' << std::endl;
	}
}

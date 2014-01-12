#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <iomanip>
#include "checker/checker.hpp"

int checker::compile(
    std::string const &compile_script_filename, 
    std::string const &solution_filename, 
    std::string const &compiled_filename
){
	pid_t childpid = fork();
	if(childpid == -1)
		return -1;
	if(childpid == 0)
	{
		if(descriptor_setup() != 0)
			exit(-1);
		execl(compile_script_filename.c_str(), compile_script_filename.c_str(), solution_filename.c_str(), compiled_filename.c_str(), NULL);
		exit(-1);
	}
	int status;
	for(int i = 0; i < 12; ++i) // wait 12 * 5 == 60 seconds
	{
		sleep(5);
		int result = waitpid(childpid, &status, WNOHANG);
		if(result != -1)
		{
			if(!WIFEXITED(status))
				return -1;
			return WEXITSTATUS(status);
		}
	}
	// compilation is longer than 60 seconds, kill it with fire
	kill(childpid, SIGKILL);
	return -1;
}
test_result checker::test_solution(
    std::string const &submissionid, 
    std::string const &solution_filename,
    std::string const &run_script_filename, 
    test_data const &test){
	std::stringstream ss;
	ss.str(test.timelimit);
	int timelimit;
	ss >> timelimit;
	ss.str(test.memlimit);
	int memlimit;
	ss >> memlimit;
	struct timespec tim;
	clock_gettime(CLOCK_MONOTONIC, &tim);
	long timetaken = tim.tv_nsec;
	pid_t childpid = fork();
	if(childpid == -1)
		return test_result(submissionid, "INT", test.testid, "0.00");
	if(childpid == 0)
	{
		if(descriptor_setup() != 0)
			exit(-1);
		struct rlimit lim;
		lim.rlim_cur = memlimit * 1024 * 1024;
		lim.rlim_max = memlimit * 1024 * 1024;
		setrlimit(RLIMIT_AS, &lim);
		lim.rlim_cur = 1024 * 1024 * 1024;
		lim.rlim_max = 1024 * 1024 * 1024;
		setrlimit(RLIMIT_FSIZE, &lim);
		lim.rlim_cur = timelimit;
		lim.rlim_max = timelimit + 1;
		setrlimit(RLIMIT_CPU, &lim);
		execl(run_script_filename.c_str(), run_script_filename.c_str(), solution_filename.c_str(), test.infilename.c_str(), "tmpresult", NULL);
		exit(-1);
	}
	int result, status;
	do
	{
		result = waitpid(childpid, &status, 0);
	} while(result == -1 && errno == EINTR);
	clock_gettime(CLOCK_MONOTONIC, &tim);
	timetaken = tim.tv_nsec - timetaken;
	ss.str(std::string());
	ss << std::fixed << std::setprecision(2) << timetaken / 1000000.;
	if(WIFSIGNALED(status))
	{
		if(WTERMSIG(status) == SIGXCPU)
			return test_result(submissionid, "TLE", test.testid, ss.str());
		return test_result(submissionid, "RTE", test.testid, ss.str());
	}
	if(WEXITSTATUS(status) != 0)
		return test_result(submissionid, "RTE", test.testid, ss.str());
	childpid = fork();
	if(childpid == -1)
		return test_result(submissionid, "INT", test.testid, "0.00");
	if(childpid == 0)
	{
		if(descriptor_setup() != 0)
			exit(-1);
		execlp("diff", "diff", "-bq", "tmpresult", "outfilename", NULL);
		exit(-1);
	}
	do
	{
		result = waitpid(childpid, &status, 0);
	} while(result == -1 && errno == EINTR);
	if(WIFSIGNALED(status) || (WEXITSTATUS(status) != 0 && WEXITSTATUS(status) != 1))
		return test_result(submissionid, "INT", test.testid, ss.str());
	if(WEXITSTATUS(status) == 1)
		return test_result(submissionid, "ANS", test.testid, ss.str());
	return test_result(submissionid, "OK", test.testid, ss.str());
}

int checker::descriptor_setup(){
	DIR* dirp = opendir("/proc/self/fd");
	if(dirp == NULL)
		return -1;
	int fd = dirfd(dirp);
	while(true)
	{
		struct dirent* direntp = readdir(dirp);
		if(direntp == NULL)
			break;
		else if(direntp->d_name[0] == '.')
			continue;
		std::istringstream ss;
		ss.str(direntp->d_name);
		int closfd;
		ss >> closfd;
		if(closfd != fd)
			close(closfd);
	}
	closedir(dirp);
	int infd = open("/dev/null", O_RDONLY);
	if(infd != 0)
	{
		dup2(infd, 0);
		close(infd);
	}
	int outfd = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if(outfd != 1)
	{
		dup2(outfd, 1);
		close(outfd);
	}
	dup2(1, 2);
	return 0;
}

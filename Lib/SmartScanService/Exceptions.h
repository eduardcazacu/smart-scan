#pragma once
/*
Custom exceptions for the Smart Scan Service

Written by Eduard Cazacu in December 2020
*/

#include <exception>
#include <string>

namespace SmartScan {
	class ex_smartScan : public std::exception {
	public:
		ex_smartScan(const char* what_arg, const char* function, const char* file) :
			what_arg{ what_arg }, function{ function }, file{ file }
		{
		}
		const char* get_file() {
			return this->file;
		}

		const char* get_function() {
			return this->function;
		}

		const char* what()
		{
			return this->what_arg;
		}
	private:
		const char* what_arg;
		const char* function;
		const char* file;
	};

	class ex_scan : public std::exception {
	public:
		ex_scan(const char* what_arg, const char* function, const char* file) :
			what_arg{ what_arg }, function{ function }, file{ file }
		{
		}
		const char* get_file() {
			return this->file;
		}

		const char* get_function() {
			return this->function;
		}

		const char* what()
		{
			return this->what_arg;
		}
	private:
		const char* what_arg;
		const char* function;
		const char* file;
	};

	class ex_trakStar : public std::exception {
	public:
		ex_trakStar(const char* what_arg, const char* function, const char* file) :
			what_arg{ what_arg }, function{ function }, file{ file }
		{
		}
		ex_trakStar(std::string what_arg, const char* function, const char* file) :
			what_arg{ what_arg.c_str() }, function{ function }, file{ file }
		{
		}
		const char* get_file() {
			return this->file;
		}

		const char* get_function() {
			return this->function;
		}

		const char* what()
		{
			return this->what_arg;
		}
	private:
		const char* what_arg;
		const char* function;
		const char* file;
	};
}

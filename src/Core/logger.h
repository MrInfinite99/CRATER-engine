#pragma once
#include<iostream>
#include<vector>
#include<string>
#include <mutex>

namespace CRATER {
	class Log {
	public:
		enum class Level {
			Info,
			Warn,
			Error
		};


		struct Line {
			Level level;
			std::string text;
		};

		static void write(Level lvl, std::string text) {
			std::scoped_lock lock(s_mutex);

			s_lines.push_back({ lvl,std::move(text) });

			if (s_lines.size() > kMax) s_lines.erase(s_lines.begin(), s_lines.begin() + kMax / 4);
		}

		static void clear() {
			std::scoped_lock lock(s_mutex);
			s_lines.clear();
		}

		static std::vector<Line> snapshot() {
			std::scoped_lock lock(s_mutex);
			return s_lines;                 // copies under the lock; lock releases on return
		}
	private:
		static constexpr size_t kMax = 4096;
		static inline std::vector<Line> s_lines;
		static inline std::mutex s_mutex;
		/*
		* the validation callback can fire from any thread, 
		and your streambuf may too. That's why 
		Log::write takes a mutex
		*/
	};

	class LogStreambuf : public std::streambuf {
	public:
		LogStreambuf(Log::Level lvl, std::streambuf* echo) : m_lvl(lvl), m_echo(echo) {}
	protected:
		int overflow(int c) override {
			if (m_echo) m_echo->sputc(char(c));          // still echo to the real console
			if (c == '\n') { Log::write(m_lvl, m_buf); m_buf.clear(); }
			else m_buf.push_back(char(c));
			return c;
		}
	private:
		Log::Level m_lvl;
		std::string m_buf;
		std::streambuf* m_echo;
	};


}
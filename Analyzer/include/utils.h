#ifndef __CMA_UTILS_H__
#define __CMA_UTILS_H__

#include <chrono>
#include <filesystem>

#ifdef _WIN32
#include<windows.h>
#else
#include <termios.h> // for tcgetattr(), tcsetattr(), and struct termios
#include <unistd.h>  // for STDIN_FILENO
#endif

namespace filesystem = std::filesystem;

namespace CMA
{
    namespace Utils
    {

        static int64_t getCurTime()
        {
            long long now = std::chrono::steady_clock::now().time_since_epoch().count();
            return now / 1000000;
        }

        static int64_t getCurTimestamp()
        {

            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }

        static bool removeFile(const std::string &filename)

        {
            if (filesystem::exists(filename))
            {
                filesystem::remove(filename);
                return true;
            }
            else
            {
                return false;
            }
        }

        static bool mkdirs(const std::string &path)

        {

            if (filesystem::exists(path))
            {
                return true;
            }
            else
            {
                return filesystem::create_directories(path);
            }
        }

        static bool is_escape_key_pressed()
        {
#ifdef _WIN32
            // On Windows, check if the escape key or the letter 'q' is pressed.
            bool pressed = GetAsyncKeyState(VK_ESCAPE) || GetAsyncKeyState('q') || GetAsyncKeyState('Q');
            return pressed;
#else
            // On Unix-like systems, switch to non-blocking input mode and check if the
            // escape key or the letter 'q' is pressed.
            struct termios old_tio, new_tio;
            tcgetattr(STDIN_FILENO, &old_tio);
            new_tio = old_tio;
            new_tio.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
            int c = getchar();
            tcflush(STDIN_FILENO, TCIFLUSH); // 清空输入缓冲区
            tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
            return c == 27 || c == 'q';
#endif
        }

        static bool is_key_pressed(int vKey)
        {
#ifdef _WIN32
            bool pressed = GetAsyncKeyState(vKey);
            FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
            // On Windows, check if the escape key or the letter 'q' is pressed.
            return pressed;
#else
            // On Unix-like systems, switch to non-blocking input mode and check if the
            // escape key or the letter 'q' is pressed.
            struct termios old_tio, new_tio;
            tcgetattr(STDIN_FILENO, &old_tio);
            new_tio = old_tio;
            new_tio.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
            int c = getchar();
            tcflush(STDIN_FILENO, TCIFLUSH); // 清空输入缓冲区
            tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
            return c == vKey;
#endif
        }

    } // namespace Utils

} // namespace CMA

#endif // !__CMA_UTILS_H__
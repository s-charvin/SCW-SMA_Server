
#ifndef __CMA_CONFIGMANAGE_H__
#define __CMA_CONFIGMANAGE_H__

#include <string>
#include <vector>
#include <mutex>
#include <memory>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

#include <string_view>
#include <shared_mutex>
#include "log.h"    // 日志
#include "json.hpp" // json 解析

using namespace std;
using json = nlohmann::json;

namespace CMA
{

    namespace ConfigManage
    {
        namespace utils
        {

            /**
             * @brief: 获取参数值的类型名称(typeinfo)
             */

#if defined(__GNUC__)
            template <class T>
            const char *type_to_name()
            {
                const char *s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
                return s_name;
            }
#else
            template <class T>
            const char *type_to_name()
            {
                return typeid(T).name();
            }
#endif

            // constexpr auto get_type_name()
            // {
            //     constexpr std::string_view fully_name = __PRETTY_FUNCTION__;
            //     constexpr std::size_t begin = [&]()
            //     {
            //         for (std::size_t i = 0; i < fully_name.size(); i++)
            //             if (fully_name[i] == '=')
            //                 return i + 2;
            //     }();
            //     constexpr std::size_t end = [&]()
            //     {
            //         for (std::size_t i = 0; i < fully_name.size(); i++)
            //             if (fully_name[i] == ']')
            //                 return i;
            //     }();
            //     constexpr auto type_name_view = fully_name.substr(begin, end - begin);
            //     constexpr auto indices = std::make_index_sequence<type_name_view.size()>();
            //     constexpr auto type_name = [&]<std::size_t... indices>(std::integer_sequence<std::size_t, indices...>)
            //     {
            //         constexpr auto str = ConfigManage::utils::static_string<type_name_view[indices]..., '\0'>();
            //         return str;
            //     }
            //     (indices);
            //     return type_name;
            // }

            template <typename To, typename From>
            struct Converter
            {
            };

            // to numeric
            template <typename From>
            struct Converter<int, From>
            {
                static int convert(const From &from)
                {
                    return std::atoi(from);
                }
            };

            template <typename From>
            struct Converter<long, From>
            {
                static long convert(const From &from)
                {
                    return std::atol(from);
                }
            };

            template <typename From>
            struct Converter<long long, From>
            {
                static long long convert(const From &from)
                {
                    return std::atoll(from);
                }
            };

            template <typename From>
            struct Converter<double, From>
            {
                static double convert(const From &from)
                {
                    return std::atof(from);
                }
            };

            template <typename From>
            struct Converter<float, From>
            {
                static float convert(const From &from)
                {
                    return (float)std::atof(from);
                }
            };

            // to bool
            template <typename From>
            struct Converter<bool, From>
            {
                static typename std::enable_if<std::is_integral<From>::value, bool>::type convert(From from)
                {
                    return !!from;
                }
            };

            static bool checkbool(const char *from, const size_t len, const char *s)
            {
                for (size_t i = 0; i < len; i++)
                {
                    if (from[i] != s[i])
                    {
                        return false;
                    }
                }

                return true;
            }

            static bool convert(const char *from)
            {
                const unsigned int len = static_cast<unsigned int>(strlen(from));
                if (len != 4 && len != 5)
                    throw std::invalid_argument("argument is invalid");

                bool r = true;
                if (len == 4)
                {
                    r = checkbool(from, len, "true");

                    if (r)
                        return true;
                }
                else
                {
                    r = checkbool(from, len, "false");

                    if (r)
                        return false;
                }

                throw std::invalid_argument("argument is invalid");
            }

            template <>
            struct Converter<bool, std::string>
            {
                static bool convert(const std::string &from)
                {
                    return ConfigManage::utils::convert(from.c_str());
                }
            };

            template <>
            struct Converter<bool, const char *>
            {
                static bool convert(const char *from)
                {
                    return ConfigManage::utils::convert(from);
                }
            };

            template <>
            struct Converter<bool, char *>
            {
                static bool convert(char *from)
                {
                    return ConfigManage::utils::convert(from);
                }
            };

            template <unsigned N>
            struct Converter<bool, const char[N]>
            {
                static bool convert(const char (&from)[N])
                {
                    return ConfigManage::utils::convert(from);
                }
            };

            template <unsigned N>
            struct Converter<bool, char[N]>
            {
                static bool convert(const char (&from)[N])
                {
                    return ConfigManage::utils::convert(from);
                }
            };

            template <typename From>
            struct Converter<std::string, From>
            {
                static string convert(const From &from)
                {
                    return std::to_string(from);
                }
            };

            template <typename To, typename From>
            typename std::enable_if<!std::is_same<To, From>::value, To>::type lexical_cast(const From &from)
            {
                return ConfigManage::utils::Converter<To, From>::convert(from);
            }
            template <typename To, typename From>
            typename std::enable_if<std::is_same<To, From>::value, To>::type lexical_cast(const From &from)
            {
                return from;
            }

            template <char... args>
            struct static_string
            {
                static constexpr const char str[] = {args...};
                operator const char *() const { return static_string::str; }
            };

        }

        /**
         * @brief: 配置参数基类
         * @details: 用于统一管理的所有配置参数的基类, 内含 m_name, m_description 两个成员变量
         */

        class ConfigBase
        {
        public:
            using ptr = std::shared_ptr<ConfigBase>; // 定义当前类的智能指针类型, 方便调用
            ConfigBase(const std::string &name, const std::string &description = "")
                : m_name(name), m_description(description)
            {
                std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
            }
            virtual ~ConfigBase() = default;

            /**
             * @brief: 返回配置参数名称
             */
            const std::string &getName() const { return m_name; }

            /**
             * @brief: 返回配置参数的描述
             */
            const std::string &getDescription() const { return m_description; }

            /**
             * @brief: 返回配置参数值的类型名称
             */
            virtual std::string getTypeName() const = 0;

            /**
             * @brief: 将配置参数值转换为字符串
             */
            virtual std::string ValtoStr() = 0;

        protected:
            std::string m_name;        // 参数名称
            std::string m_description; // 参数描述
        };

        /**
         * @brief: 参数配置类模板
         * @details: 构造 T 类型参数的具体类, 继承自 ConfigBase, 内含 m_name, m_description, m_val, m_mutex, m_cbs 四个成员变量
         */

        template <class T>
        class Config : public ConfigBase
        {
        public:
            using ptr = std::shared_ptr<Config>;                                            // 定义当前类的智能指针类型， 方便调用
            typedef std::function<void(const T &old_value, const T &new_value)> OnChangeCb; // 定义回调函数的类型模板

            Config(const std::string &name, const T &default_value, const std::string &description = "") : ConfigBase(name, description), m_val(default_value){};
            ~Config() override = default;

        private:
            std::shared_mutex m_mutex;     // 参数值的读写锁
            T m_val;                       // 参数值
            std::vector<OnChangeCb> m_cbs; // 参数值变化时的回调函数组

        public:
            /**
             * @brief: 获取当前参数的值
             */
            const T getValue()
            {
                std::shared_lock lock(m_mutex);
                return m_val;
            }

            /**
             * @brief: 设置当前参数的值
             */
            void setValue(const T &v)
            {
                std::unique_lock lock(m_mutex);
                if (v == m_val)
                    return;
                for (auto &i : m_cbs)
                    i(m_val, v);
                m_val = v;
            }

            /**
             * @brief 设置当前参数的描述信息
             */
            void setDescription(const std::string &v)
            {
                std::unique_lock lock(m_mutex);
                m_description = v;
            }

            /**
             * brief: 返回参数值的类型名称(typeinfo)
             */
            std::string getTypeName() const override { return utils::type_to_name<T>(); };

            /**
             * @brief: 将参数值转换为字符串
             */
            std::string ValtoStr() override
            {

                try
                {
                    std::shared_lock lock(m_mutex);
                    return ConfigManage::utils::lexical_cast<std::string>(m_val);
                }
                catch (std::exception &e)
                {
                    LOG(ERROR) << "Config::toString exception " << e.what()
                               << ", convert: " << getTypeName() << " to string"
                               << ", name=" << m_name;
                }
                return "Convert Error.";
            }

        public:
            /**
             * @brief: 添加变化回调函数
             */
            void addCallback(OnChangeCb cb)
            {
                std::unique_lock lock(m_mutex);
                m_cbs.push_back(cb);
            };

            /**
             * @brief: 清理所有的回调函数
             */
            void clearCallback()
            {
                std::unique_lock lock(m_mutex);
                m_cbs.clear();
            };
        };

        /**
         * @brief: 参数管理类
         * @details: 用于管理配置参数, 以及对应的回调函数, 内含 m_mutex, m_cfgs 两个成员变量
         */
        class ConfigManager
        {
        public:
            ConfigManager() = default; // 构造函数
            ~ConfigManager();          // 析构函数

        private:
            std::shared_mutex m_mutex;                               // 参数管理类的读写锁
            std::unordered_map<std::string, ConfigBase::ptr> m_cfgs; // 参数列表

        public:
            void clear();

            template <class T>
            typename Config<T>::ptr set(const std::string &name,
                                        const T &value,
                                        const std::string &description)
            {
                std::unique_lock lock(m_mutex);
                auto it = m_cfgs.find(name);
                if (it != m_cfgs.end())
                {
                    auto config_tmp = std::dynamic_pointer_cast<Config<T>>(it->second);
                    if (config_tmp)
                    {
                        LOG(INFO) << name << " has been created, so modify its value."
                                  << "original: value(" << config_tmp->getValue() << ") type: ()" << config_tmp->getTypeName() << ") description: (" << description << ")";
                        config_tmp->setValue(value);
                        config_tmp->setDescription(description);

                        LOG(INFO) << "new: value(" << config_tmp->getValue() << ") type: (" << config_tmp->getTypeName() << ") description: (" << description << ")";

                        return config_tmp;
                    }
                    else
                    {
                        LOG(ERROR) << name << " has been created, but type is not match."
                                   << " original: type(" << it->second->getTypeName() << ") new: type(" << utils::type_to_name<T>() << ")" << std::endl;
                        std::exit(1);
                        return nullptr;
                    }
                }

                typename Config<T>::ptr v(new Config<T>(name, value, description));
                m_cfgs[name] = v;
                LOG(INFO) << "create new config: name(" << name << ") value(" << v->getValue() << ") type(" << v->getTypeName() << ") description(" << v->getDescription() << ")";
                return v;
            }

            template <class T>
            typename Config<T>::ptr set(const std::string &name,
                                        const T &value)
            {
                std::unique_lock lock(m_mutex);
                auto it = m_cfgs.find(name);
                if (it != m_cfgs.end())
                {
                    auto config_tmp = std::dynamic_pointer_cast<Config<T>>(it->second);
                    if (config_tmp)
                    {
                        LOG(INFO) << name << " has been created, so modify its value. ";
                        LOG(INFO) << "original: value(" << config_tmp->getValue() << ") type: (" << config_tmp->getTypeName() << ") description: (" << config_tmp->getDescription() << ")";
                        config_tmp->setValue(value);
                        LOG(INFO) << "new: value(" << config_tmp->getValue() << ") type: (" << config_tmp->getTypeName() << ") description: (" << config_tmp->getDescription() << ")";
                        return config_tmp;
                    }
                    else
                    {
                        LOG(ERROR) << name << " has been created, but type is not match."
                                   << " original: type(" << it->second->getTypeName() << ") new: type(" << utils::type_to_name<T>() << ")" << std::endl;
                        std::exit(1);
                        return nullptr;
                    }
                }
                typename Config<T>::ptr v(new Config<T>(name, value));
                m_cfgs[name] = v;
                LOG(INFO) << "create new config: name(" << name << ") value(" << v->getValue() << ") type(" << v->getTypeName() << ") description(" << v->getDescription() << ")";
                return v;
            }

            /**
             * @brief: 类函数模板, 用于查找指定名称的配置参数
             */
            template <class T>
            typename Config<T>::ptr get(const std::string &name)
            {
                auto it = m_cfgs.find(name);
                return it == m_cfgs.end() ? nullptr : std::dynamic_pointer_cast<Config<T>>(it->second);
            }

            const std::unordered_map<std::string, CMA::ConfigManage::ConfigBase::ptr> get()
            {
                return m_cfgs;
            }

            // /**
            //  * @brief: 遍历配置模块里面所有配置项
            //  */

            // void visit(std::function<void(ConfigBase::ptr)> cb);

            friend std::ostream &operator<<(std::ostream &os, ConfigManager &cm)
            {
                std::shared_lock lock(cm.m_mutex);
                os << "ConfigManager: " << std::endl;
                for (const std::map<std::string, ConfigBase::ptr>::value_type &pair : cm.m_cfgs)
                {
                    os << "Parameter name: " << pair.first << std::endl;
                    os << "  Type: " << pair.second->getTypeName() << std::endl;
                    os << "  Value: " << pair.second->ValtoStr() << std::endl;
                    os << "  Description: " << pair.second->getDescription() << std::endl;
                }
                return os;
            }
        };

        /* 程序运行全局参数配置对象 */
        class CMAConfig
        {

        public:
            static CMAConfig *instance(const std::string &file) // 单例模式
            {
                // 如果实例还未创建，则创建一个新的实例
                if (m_instance == nullptr)
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_instance == nullptr)
                    {
                        m_instance = new CMAConfig(file);
                    }
                }
                return m_instance;
            }

            CMAConfig(const CMAConfig &) = delete;            // 禁止拷贝构造
            CMAConfig &operator=(const CMAConfig &) = delete; // 禁止赋值构造

            // 垃圾回收
            class CGabor
            {
            public:
                ~CGabor(){
                    if (CMAConfig::m_instance){
                        delete CMAConfig::m_instance;
                        CMAConfig::m_instance = nullptr;
                    }
                }
            };


        private:
            /* 变量初始化函数声明, (const char *):指向常量的指针 */
            CMAConfig(const std::string &file); // 私有构造函数
            ~CMAConfig();                       // 私有析构函数

        public:
            ConfigManager *config_mgr = new ConfigManager(); /* 配置管理器 */
            /*管理器所管理的参数名列表 */
            const std::vector<std::string> param_names = {
                "server_host", "server_port", "cache_dir", "control_executor_maxNum",
                "support_hardware_videoDecode", "support_hardware_videoEncode",
                "algorithm_device", "algorithm_instanceNum"};

            bool m_state = false; /* 参数解析成功标志 */
            void show_help();     /* 展示帮助信息 */
            
        private:
            static CMAConfig *m_instance;
            static std::mutex m_mutex;
            static CGabor m_gabor;
            /* 解析配置文件 */
            const json parse_config(const std::string &file);
            /* 检查参数配置是否完整 */
            void check_config(const json &j);
        };

        class TaskConfig
        {
        public:
            TaskConfig(json &root);
            TaskConfig(std::string id, std::string phsa, std::string plsa, bool phse, std::string ac, int64_t ami, int priority, int64_t delay);
            ~TaskConfig();

        public:
            ConfigManager *config_mgr = new ConfigManager();

            /*管理器所管理的参数名列表 */
            const std::vector<std::string> param_names = {
                "id", "priority", "delay", "pullStream_address", "pushStream_enable", "pushStream_address",
                "algorithmCode", "process_minInterval", "processFps",
                "executorStartTimestamp",
                "videoWidth", "videoHeight", "videoChannel", "videoIndex", "videoFps",
                "audioIndex", "audioSampleRate", "audioChannel", "audioFps"};

            const std::string get_id()
            {
                return config_mgr->get<std::string>("id")->getValue();
            };

        public:
            bool checkAdd(std::string &result_msg);
            bool checkCancel(std::string &result_msg);

        private:
            bool m_state = false; /* 参数解析成功标志 */
            bool check_config();
        };

    }

}
#endif // !__CMA_CONFIGMANAGE_H__
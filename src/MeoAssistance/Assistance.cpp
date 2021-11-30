#include "Assistance.h"

#include <filesystem>
#include <time.h>

#include <json.h>
#include <opencv2/opencv.hpp>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "Logger.hpp"
#include "Resource.h"

#include "CreditShoppingTask.h"
#include "InfrastDormTask.h"
#include "InfrastInfoTask.h"
#include "InfrastMfgTask.h"
#include "InfrastOfficeTask.h"
#include "InfrastPowerTask.h"
#include "InfrastReceptionTask.h"
#include "InfrastTradeTask.h"
#include "ProcessTask.h"
#include "RecruitTask.h"

using namespace asst;

Assistance::Assistance(AsstCallback callback, void* callback_arg)
    : m_callback(callback), m_callback_arg(callback_arg)
{
    LogTraceFunction;

    bool resource_ret = resource.load(utils::get_resource_dir());
    if (!resource_ret) {
        const std::string& error = resource.get_last_error();
        log.error("resource broken", error);
        if (m_callback == nullptr) {
            throw error;
        }
        json::value callback_json;
        callback_json["type"] = "resource broken";
        callback_json["what"] = error;
        m_callback(AsstMsg::InitFaild, callback_json, m_callback_arg);
        throw error;
    }

    m_working_thread = std::thread(std::bind(&Assistance::working_proc, this));
    m_msg_thread = std::thread(std::bind(&Assistance::msg_proc, this));
}

Assistance::~Assistance()
{
    LogTraceFunction;

    m_thread_exit = true;
    m_thread_idle = true;
    m_condvar.notify_all();
    m_msg_condvar.notify_all();

    if (m_working_thread.joinable()) {
        m_working_thread.join();
    }
    if (m_msg_thread.joinable()) {
        m_msg_thread.join();
    }
}

bool asst::Assistance::catch_default()
{
    LogTraceFunction;

    auto& opt = resource.cfg().get_options();
    switch (opt.connect_type) {
    case ConnectType::Emulator:
        return catch_emulator();
    case ConnectType::Custom:
        return catch_custom();
    default:
        return false;
    }
}

bool Assistance::catch_emulator(const std::string& emulator_name)
{
    LogTraceFunction;

    stop();

    bool ret = false;
    //std::string cor_name = emulator_name;
    auto& cfg = resource.cfg();

    std::unique_lock<std::mutex> lock(m_mutex);

    // 自动匹配模拟器，逐个找
    if (emulator_name.empty()) {
        for (const auto& [name, info] : cfg.get_emulators_info()) {
            ret = ctrler.try_capture(info);
            if (ret) {
                //cor_name = name;
                break;
            }
        }
    }
    else { // 指定的模拟器
        auto& info = cfg.get_emulators_info().at(emulator_name);
        ret = ctrler.try_capture(info);
    }

    m_inited = ret;
    return ret;
}

bool asst::Assistance::catch_custom()
{
    LogTraceFunction;

    stop();

    bool ret = false;
    auto& cfg = resource.cfg();

    std::unique_lock<std::mutex> lock(m_mutex);

    EmulatorInfo remote_info = cfg.get_emulators_info().at("Custom");

    ret = ctrler.try_capture(remote_info, true);

    m_inited = ret;
    return ret;
}

bool asst::Assistance::catch_fake()
{
    LogTraceFunction;

    stop();

    m_inited = true;
    return true;
}

bool asst::Assistance::append_fight(int mecidine, int stone, int times, bool only_append)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    auto task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this);
    task_ptr->set_task_chain("Fight");
    task_ptr->set_tasks({ "SanityBegin" });
    task_ptr->set_times_limit("MedicineConfirm", mecidine);
    task_ptr->set_times_limit("StoneConfirm", stone);
    task_ptr->set_times_limit("StartButton1", times);

    m_tasks_queue.emplace(task_ptr);

    if (!only_append) {
        return start(false);
    }

    return true;
}

bool asst::Assistance::append_award(bool only_append)
{
    return append_process_task("AwardBegin", "ReceiveAward");
}

bool asst::Assistance::append_visit(bool only_append)
{
    return append_process_task("VisitBegin", "Visit");
}

bool asst::Assistance::append_mall(bool with_shopping, bool only_append)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    const std::string task_chain = "Mall";

    append_process_task("MallBegin", task_chain);

    if (with_shopping) {
        auto shopping_task_ptr = std::make_shared<CreditShoppingTask>(task_callback, (void*)this);
        shopping_task_ptr->set_task_chain(task_chain);
        m_tasks_queue.emplace(shopping_task_ptr);
    }

    if (!only_append) {
        return start(false);
    }

    return true;
}

bool Assistance::append_process_task(const std::string& task, std::string task_chain, int retry_times)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    //std::unique_lock<std::mutex> lock(m_mutex);

    if (task_chain.empty()) {
        task_chain = task;
    }

    auto task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this);
    task_ptr->set_task_chain(task_chain);
    task_ptr->set_tasks({ task });
    task_ptr->set_retry_times(retry_times);

    m_tasks_queue.emplace(task_ptr);

    //if (!only_append) {
    //    return start(false);
    //}

    return true;
}

#ifdef LOG_TRACE
bool Assistance::append_debug()
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    //{
    //    constexpr static const char* DebugTaskChain = "Debug";
    //    auto shift_task_ptr = std::make_shared<InfrastProductionTask>(task_callback, (void*)this);
    //    shift_task_ptr->set_work_mode(infrast::WorkMode::Aggressive);
    //    shift_task_ptr->set_facility("Mfg");
    //    shift_task_ptr->set_product("CombatRecord");
    //    shift_task_ptr->set_task_chain(DebugTaskChain);
    //    m_tasks_queue.emplace(shift_task_ptr);
    //}

    return true;
}
#endif

bool Assistance::start_recruit_calc(const std::vector<int>& required_level, bool set_time)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    auto task_ptr = std::make_shared<RecruitTask>(task_callback, (void*)this);
    task_ptr->set_param(required_level, set_time);
    task_ptr->set_retry_times(OpenRecruitTaskRetyrTimesDefault);
    task_ptr->set_task_chain("OpenRecruit");
    m_tasks_queue.emplace(task_ptr);

    return start(false);
}

bool asst::Assistance::append_infrast(infrast::WorkMode work_mode, const std::vector<std::string>& order, UsesOfDrones uses_of_drones, double dorm_threshold, bool only_append)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    // 保留接口，目前强制按激进模式进行换班
    work_mode = infrast::WorkMode::Aggressive;

    constexpr static const char* InfrastTaskCahin = "Infrast";

    // 这个流程任务，结束的时候是处于基建主界面的。既可以用于进入基建，也可以用于从设施里返回基建主界面

    auto append_infrast_begin = [&]() {
        append_process_task("InfrastBegin", InfrastTaskCahin);
    };

    append_infrast_begin();

    auto info_task_ptr = std::make_shared<InfrastInfoTask>(task_callback, (void*)this);
    info_task_ptr->set_work_mode(work_mode);
    info_task_ptr->set_task_chain(InfrastTaskCahin);
    info_task_ptr->set_mood_threshold(dorm_threshold);

    m_tasks_queue.emplace(info_task_ptr);

    // 因为后期要考虑多任务间的联动等，所以这些任务的声明暂时不放到for循环中
    auto mfg_task_ptr = std::make_shared<InfrastMfgTask>(task_callback, (void*)this);
    mfg_task_ptr->set_work_mode(work_mode);
    mfg_task_ptr->set_task_chain(InfrastTaskCahin);
    mfg_task_ptr->set_mood_threshold(dorm_threshold);
    auto trade_task_ptr = std::make_shared<InfrastTradeTask>(task_callback, (void*)this);
    trade_task_ptr->set_work_mode(work_mode);
    trade_task_ptr->set_task_chain(InfrastTaskCahin);
    trade_task_ptr->set_mood_threshold(dorm_threshold);
    auto power_task_ptr = std::make_shared<InfrastPowerTask>(task_callback, (void*)this);
    power_task_ptr->set_work_mode(work_mode);
    power_task_ptr->set_task_chain(InfrastTaskCahin);
    power_task_ptr->set_mood_threshold(dorm_threshold);
    auto office_task_ptr = std::make_shared<InfrastOfficeTask>(task_callback, (void*)this);
    office_task_ptr->set_work_mode(work_mode);
    office_task_ptr->set_task_chain(InfrastTaskCahin);
    office_task_ptr->set_mood_threshold(dorm_threshold);
    auto recpt_task_ptr = std::make_shared<InfrastReceptionTask>(task_callback, (void*)this);
    recpt_task_ptr->set_work_mode(work_mode);
    recpt_task_ptr->set_task_chain(InfrastTaskCahin);
    recpt_task_ptr->set_mood_threshold(dorm_threshold);

    auto dorm_task_ptr = std::make_shared<InfrastDormTask>(task_callback, (void*)this);
    dorm_task_ptr->set_work_mode(work_mode);
    dorm_task_ptr->set_task_chain(InfrastTaskCahin);
    dorm_task_ptr->set_mood_threshold(dorm_threshold);

    for (const auto& facility : order) {
        if (facility == "Dorm") {
            m_tasks_queue.emplace(dorm_task_ptr);
        }
        else if (facility == "Mfg") {
            m_tasks_queue.emplace(mfg_task_ptr);
            if (UsesOfDrones::DronesMfg & uses_of_drones) {
                append_process_task("DroneAssist-MFG", InfrastTaskCahin);
            }
        }
        else if (facility == "Trade") {
            m_tasks_queue.emplace(trade_task_ptr);
            if (UsesOfDrones::DronesTrade & uses_of_drones) {
                append_process_task("DroneAssist-Trade", InfrastTaskCahin);
            }
        }
        else if (facility == "Power") {
            m_tasks_queue.emplace(power_task_ptr);
        }
        else if (facility == "Office") {
            m_tasks_queue.emplace(office_task_ptr);
        }
        else if (facility == "Reception") {
            m_tasks_queue.emplace(recpt_task_ptr);
        }
        else {
            log.error("append_infrast | Unknown facility", facility);
        }
        append_infrast_begin();
    }

    if (!only_append) {
        return start(false);
    }

    return true;
}

void asst::Assistance::set_penguin_id(const std::string& id)
{
    auto& opt = resource.cfg().get_options();
    if (id.empty()) {
        opt.penguin_report_extra_param.clear();
    }
    else {
        opt.penguin_report_extra_param = "-H \"authorization: PenguinID " + id + "\"";
    }
}

bool asst::Assistance::start(bool block)
{
    LogTraceFunction;
    log.trace("Start |", block ? "block" : "non block");

    if (!m_thread_idle || !m_inited) {
        return false;
    }
    std::unique_lock<std::mutex> lock;
    if (block) { // 外部调用
        lock = std::unique_lock<std::mutex>(m_mutex);
    }

    m_thread_idle = false;
    m_condvar.notify_one();

    return true;
}

bool Assistance::stop(bool block)
{
    LogTraceFunction;
    log.trace("Stop |", block ? "block" : "non block");

    m_thread_idle = true;

    std::unique_lock<std::mutex> lock;
    if (block) { // 外部调用
        lock = std::unique_lock<std::mutex>(m_mutex);
    }
    decltype(m_tasks_queue) empty;
    m_tasks_queue.swap(empty);

    clear_cache();

    return true;
}

void Assistance::working_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {
        //LogTraceScope("Assistance::working_proc Loop");
        std::unique_lock<std::mutex> lock(m_mutex);

        if (!m_thread_idle && !m_tasks_queue.empty()) {
            auto start_time = std::chrono::system_clock::now();

            auto task_ptr = m_tasks_queue.front();
            task_ptr->set_exit_flag(&m_thread_idle);

            bool ret = task_ptr->run();
            m_tasks_queue.pop();

            std::string cur_taskchain = task_ptr->get_task_chain();
            json::value task_json = json::object{
                {"task_chain", cur_taskchain}
            };

            if (ret) {
                if (m_tasks_queue.empty() || cur_taskchain != m_tasks_queue.front()->get_task_chain()) {
                    task_callback(AsstMsg::TaskChainCompleted, task_json, this);
                }
                if (m_tasks_queue.empty()) {
                    task_callback(AsstMsg::AllTasksCompleted, json::value(), this);
                }
            }
            else {
                task_callback(AsstMsg::TaskError, task_json, this);
            }

            //clear_cache();

            auto& delay = resource.cfg().get_options().task_delay;
            m_condvar.wait_until(lock, start_time + std::chrono::milliseconds(delay),
                [&]() -> bool { return m_thread_idle; });
        }
        else {
            m_thread_idle = true;
            //controller.set_idle(true);
            m_condvar.wait(lock);
        }
    }
}

void Assistance::msg_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {
        //LogTraceScope("Assistance::msg_proc Loop");
        std::unique_lock<std::mutex> lock(m_msg_mutex);
        if (!m_msg_queue.empty()) {
            // 结构化绑定只能是引用，后续的pop会使引用失效，所以需要重新构造一份，这里采用了move的方式
            auto&& [temp_msg, temp_detail] = m_msg_queue.front();
            AsstMsg msg = std::move(temp_msg);
            json::value detail = std::move(temp_detail);
            m_msg_queue.pop();
            lock.unlock();

            if (m_callback) {
                m_callback(msg, detail, m_callback_arg);
            }
        }
        else {
            m_msg_condvar.wait(lock);
        }
    }
}

void Assistance::task_callback(AsstMsg msg, const json::value& detail, void* custom_arg)
{
    log.trace("Assistance::task_callback |", msg, detail.to_string());

    Assistance* p_this = (Assistance*)custom_arg;
    json::value more_detail = detail;
    switch (msg) {
    case AsstMsg::PtrIsNull:
    case AsstMsg::ImageIsEmpty:
        p_this->stop(false);
        break;
    case AsstMsg::StageDrops:
        more_detail = p_this->organize_stage_drop(more_detail);
        break;
    default:
        break;
    }

    // Todo: 有些不需要回调给外部的消息，得在这里给拦截掉
    // 加入回调消息队列，由回调消息线程外抛给外部
    p_this->append_callback(msg, std::move(more_detail));
}

void asst::Assistance::append_callback(AsstMsg msg, json::value detail)
{
    std::unique_lock<std::mutex> lock(m_msg_mutex);
    m_msg_queue.emplace(msg, std::move(detail));
    m_msg_condvar.notify_one();
}

void Assistance::clear_cache()
{
    resource.templ().clear_hists();
}

json::value asst::Assistance::organize_stage_drop(const json::value& rec)
{
    json::value dst = rec;
    auto& item = resource.item();
    for (json::value& drop : dst["drops"].as_array()) {
        std::string id = drop["itemId"].as_string();
        int quantity = drop["quantity"].as_integer();
        item.increase_drop_count(id, quantity);
        const std::string& name = item.get_item_name(id);
        drop["itemName"] = name.empty() ? "未知材料" : name;
    }
    std::vector<json::value> statistics_vec;
    for (auto&& [id, count] : item.get_drop_count()) {
        json::value info;
        info["itemId"] = id;
        const std::string& name = item.get_item_name(id);
        info["itemName"] = name.empty() ? "未知材料" : name;
        info["count"] = count;
        statistics_vec.emplace_back(std::move(info));
    }
    // 排个序，数量多的放前面
    std::sort(statistics_vec.begin(), statistics_vec.end(),
        [](const json::value& lhs, const json::value& rhs) -> bool {
            return lhs.at("count").as_integer() > rhs.at("count").as_integer();
        });

    dst["statistics"] = json::array(std::move(statistics_vec));

    log.trace("organize_stage_drop | ", dst.to_string());

    return dst;
}

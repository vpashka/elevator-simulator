#ifndef TestControlAgent_H_
#define TestControlAgent_H_
// --------------------------------------------------------------------------
#include <string>
#include <so_5/all.hpp>
#include "Control.h"
// --------------------------------------------------------------------------
/*!
 * Т.к. Control активно взаимодействует с другими агентами (двери, лифт)
 * то для имитации (и контроля подачи команд) создаётся этот специальный тестовый агент,
 * в задачи которого входит:
 * - контроль подачи команд
 * - имитация ответов
 * - предоставление отчёта о текущей ситуации для проверки в тесте
*/
class TestControlAgent final:
	public so_5::agent_t
{
	public:
        TestControlAgent( context_t ctx
               , so_5::mbox_t control_mbox
               , so_5::mbox_t status_mbox
               , const size_t height );

        virtual ~TestControlAgent();

        // Отчёт, который запрашивает тест
        struct msg_report_t:
            public so_5::message_t
        {
            // счётчики команд
            size_t doors_open_cmd = { 0 };
            size_t doors_close_cmd = { 0 };
            size_t elevator_move_cmd = { 0 };

            Elevator::msg_move_to_level_t elevator_last_cmd = { 0 };
            Doors::msg_state_t doors { Doors::State::dClosed };

            size_t level = { 1 };
        };

        struct msg_get_report_t:
            public so_5::message_t
        {};

        struct msg_elevator_cmd_t:
            public so_5::message_t
        {
            Elevator::msg_move_to_level_t cmd;
            msg_elevator_cmd_t( const Elevator::msg_move_to_level_t& m )
                :cmd(m)
            {}
        };

    protected:

        virtual void so_define_agent() override;
        virtual void so_evt_start() override;

    private:
        so_5::mbox_t control_mbox;
        so_5::mbox_t status_mbox;
        msg_report_t report;
        const size_t height;

        size_t current_level = { 1 }; // текущий имтируемый этаж (по умолчанию первый)
        Doors::msg_state_t doors; // текущее состояние дверей
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------

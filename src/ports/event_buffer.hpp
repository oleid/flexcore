#ifndef SRC_PORTS_EVENT_BUFFER_HPP_
#define SRC_PORTS_EVENT_BUFFER_HPP_

#include <functional>
#include <memory>

#include "event_ports.hpp"

namespace fc
{

template<class event_t>
struct buffer_interface
{
	virtual event_in_port<event_t> in_events() = 0;
	virtual event_out_port<event_t> out_events() = 0;
	virtual ~buffer_interface() = default;
};

template<class event_t>
class no_buffer : public buffer_interface<event_t>
{
public:
	no_buffer()
	: 	in_event_port( [this](event_t in_event) { out_event_port.fire(in_event);})
	{
	}

	event_in_port<event_t> in_events() override
	{
		return in_event_port;
	}
	event_out_port<event_t> out_events() override
	{
		return out_event_port;
	}

private:
	event_in_port<event_t> in_event_port;
	event_out_port<event_t> out_event_port;
};

/**
 * \brief buffer for events using double buffering
 *
 * Stores events of event_t in two buffers.
 * Switches Buffers on receiving switch_tick.
 * This moves events from internal to external buffer.
 * New events are added to to the internal buffer.
 * Events from the external buffer are fired on receiving send tick.
 */
template<class event_t>
class event_buffer : public buffer_interface<event_t>
{
public:

	event_buffer()
		: in_switch_tick( [this](){ switch_buffers(); } )
		, in_send_tick( [this](){ send_events(); } )
		, in_event_port( [this](event_t in_event) { std::cout << "in event\n";intern_buffer->push_back(in_event);})
		, intern_buffer(std::make_shared<buffer_t>())
		, extern_buffer(std::make_shared<buffer_t>())
		{
		std::cout << "Zonk! event_buffer()\n";
		}

	// event in port of type void, switches buffers
	auto switch_tick() { return in_switch_tick; };
	// event in port of type void, fires external buffer
	auto send_tick() { return in_send_tick; };

	event_in_port<event_t> in_events() override
	{
		return in_event_port;
	}
	event_out_port<event_t> out_events() override
	{
		return out_event_port;
	}

protected:
	void switch_buffers()
	{
		std::cout << "event_buffer switch buffers!\n";
		// move content of intern buffer to extern, leaving content of extern buffer
		// since the buffers might be switched several times, before extern buffer is emptied.
		// otherwise we would potentially lose events on switch.
		extern_buffer->insert(
				end(*extern_buffer), begin(*intern_buffer), end(*intern_buffer));
		intern_buffer->clear();
	}

	void send_events()
	{
		std::cout << "event_buffer send_events!\n";
		std::cout << "buffer sizes " << extern_buffer->size() << ", " << intern_buffer->size() << "\n";

		for (event_t e : *extern_buffer)
			out_event_port.fire(e);

		// delete content of extern buffer, do not change capacity,
		// since we want to avoid allocations in next cycle.
		extern_buffer->clear();
	}

	event_in_port<void> in_switch_tick;
	event_in_port<void> in_send_tick;
	event_in_port<event_t> in_event_port;
	event_out_port<event_t> out_event_port;

	typedef std::vector<event_t> buffer_t;
	std::shared_ptr<buffer_t> intern_buffer;
	std::shared_ptr<buffer_t> extern_buffer;
};

} // namespace fc

#endif /* SRC_PORTS_EVENT_BUFFER_HPP_ */

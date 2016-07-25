#pragma once

#include "core/core.hpp"
#include "UUID.hpp"

namespace kinski{ namespace bluetooth
{

    typedef std::shared_ptr<class Central> CentralPtr;
    typedef std::shared_ptr<class Peripheral> PeripheralPtr;
    typedef std::function<void(CentralPtr, PeripheralPtr)> PeripheralCallback;

    class Central : public std::enable_shared_from_this<Central>
    {
    public:

        //! create a Central
        static CentralPtr create();

        //! start scanning for peripherals, optionally filter for specific service-UUIDs
        void discover_peripherals(const std::set<UUID>& the_service_uuids = {});

        //! stop scanning and perpheral-discovery
        void stop_scanning();

        //! connect a discovered perpheral
        void connect_peripheral(const PeripheralPtr &the_peripheral,
                                PeripheralCallback cb = PeripheralCallback());

        //! disconnect a previously connected perpheral
        void disconnect_peripheral(const PeripheralPtr &the_peripheral);

        //! disconnect all connected perpherals
        void disconnect_all();

        //! set the callback to fire when a new peripheral has been discovered
        void set_peripheral_discovered_cb(PeripheralCallback cb);

        //! set the callback to fire when a peripheral was successfully connected
        void set_peripheral_connected_cb(PeripheralCallback cb);

        //! set the callback to fire when a previously connected perpheral has been disconnected
        void set_peripheral_disconnected_cb(PeripheralCallback cb);

        //! return all peripherals, that have been discovered after the last call to <discover_peripherals>
        std::set<PeripheralPtr> peripherals() const;

    private:
        std::shared_ptr<struct CentralImpl> m_impl;
        Central();
    };

    class Peripheral : public std::enable_shared_from_this<Peripheral>
    {
    public:

        typedef std::function<void(const UUID&, const std::vector<uint8_t>&)> ValueUpdatedCallback;

        //! return the peripherals name, if known. will return "(unknown)" else
        const std::string& name() const;

        //! returns true, if the peripheral is currently connected to a central
        bool is_connected() const;

        //! returns true, if the peripheral is advertised as connectable
        bool connectable() const;

        //! returns the current received signal strength indicator (RSSI) in the range [-127, 0]
        int rssi() const;

        //! returns a unique, plattform-dependant identifier for this peripheral
        const std::string identifier() const;

        //! returns a backpointer to the central that created this peripheral
        CentralPtr central() const;

        //! start a GATT-service discovery for this peripheral,
        //  optionally provide a set of UUIDs to filter for
        void discover_services(const std::set<UUID>& the_uuids = {});

        //! write a GATT-characteristic, specified by its UUID
        void write_value_for_characteristic(const UUID &the_characteristic,
                                            const std::vector<uint8_t> &the_data);

        //! read a GATT-characteristic, specified by its UUID.
        //  the value will be returned through the provided callback after retrieval
        void read_value_for_characteristic(const UUID &the_characteristic,
                                           ValueUpdatedCallback cb);

        // TODO: ugly, maybe remove getter here
        ValueUpdatedCallback value_updated_cb();

        //! set the callback to fire when an observable characteristic changed its value
        void set_value_updated_cb(ValueUpdatedCallback cb);

        void add_service(const UUID& the_service_uuid);
        void add_characteristic(const UUID& the_service_uuid, const UUID& the_characteristic_uuid);

        //! return a map of all discovered services by their UUID
        //  and the corresponding list of characeristic UUIDs
        const std::map<UUID, std::set<UUID>>& known_services();

    private:
        friend CentralImpl;
        Peripheral();
        std::shared_ptr<struct PeripheralImpl> m_impl;
    };

}}//namespace

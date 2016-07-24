//
//  bluetooth.cpp
//
//  Created by Fabian on 3/5/15.
//
//

#include "bluetooth.hpp"
#include "CoreBluetooth/CoreBluetooth.h"

@interface CoreBluetoothDelegate : NSObject<CBCentralManagerDelegate, CBPeripheralDelegate>
{
    kinski::bluetooth::CentralImpl *m_central_impl;
}
- (instancetype) initWithImpl: (kinski::bluetooth::CentralImpl*) the_impl;

@property(assign) kinski::bluetooth::CentralImpl *central_impl;
@end

///////////////////////////////////////////////////////////////////////////////////////////

namespace kinski{ namespace bluetooth{
    
    ///////////////////////////////// Helpers ////////////////////////////////////////////////
    
    typedef std::map<PeripheralPtr, CBPeripheral*> PeripheralMap;
    typedef std::map<CBPeripheral*, PeripheralPtr> PeripheralReverseMap;

    PeripheralMap g_peripheral_map;
    PeripheralReverseMap g_peripheral_reverse_map;

    CBCharacteristic* find_characteristic(PeripheralPtr the_peripheral,
                                          const UUID& the_characteristic_uuid);
    CBPeripheral* find_peripheral(PeripheralPtr the_peripheral);

    std::string uuid_to_str(CBUUID *the_uuid)
    {
        return [the_uuid.UUIDString UTF8String];
    }

    CBUUID* str_to_uuid(const std::string &the_str)
    {
        return [CBUUID UUIDWithString:[NSString stringWithUTF8String:the_str.c_str()]];
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    struct PeripheralImpl
    {
        std::weak_ptr<CentralImpl> m_central_impl_ref;
        UUID uuid;
        std::string name = "unknown";
        bool connectable = false;
        int rssi;
        std::map<UUID, std::set<UUID>> known_services;
        Peripheral::ValueUpdatedCallback value_updated_cb;
    };
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    struct CentralImpl : public std::enable_shared_from_this<CentralImpl>
    {
        CBCentralManager* central_manager;

        CoreBluetoothDelegate* delegate;

        PeripheralCallback
        peripheral_discovered_cb, peripheral_connected_cb, peripheral_disconnected_cb;

        std::weak_ptr<Central> central_ref;

        CentralImpl()
        {
            delegate = [[CoreBluetoothDelegate alloc] initWithImpl:this];
            central_manager = [[CBCentralManager alloc] initWithDelegate:delegate queue:nil options:nil];
        }
        ~CentralImpl()
        {
            [central_manager stopScan];
            [central_manager dealloc];
            [delegate dealloc];
        }
        
        PeripheralPtr create_peripheral()
        {
            auto ret = PeripheralPtr(new Peripheral);
            ret->m_impl->m_central_impl_ref = shared_from_this();
            return ret;
        }
        
        void set_peripheral_name(PeripheralPtr the_peripheral, const std::string &the_name)
        {
            the_peripheral->m_impl->name = the_name;
        }
        
        void set_peripheral_connectable(PeripheralPtr the_peripheral, bool b)
        {
            the_peripheral->m_impl->connectable = b;
        }
        
        void set_peripheral_rssi(PeripheralPtr the_peripheral, int the_rssi)
        {
            the_peripheral->m_impl->rssi = the_rssi;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    CentralPtr Central::create(){ return CentralPtr(new Central()); }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    Central::Central():
    m_impl(new CentralImpl){ }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Central::discover_peripherals(const std::set<UUID>& the_service_uuids)
    {
        LOG_DEBUG << "discover_peripherals";
        m_impl->central_ref = shared_from_this();
        NSMutableArray<CBUUID *> *services = [NSMutableArray<CBUUID *> array];

        for(const auto &uuid : the_service_uuids)
        {
            [services addObject:kinski::bluetooth::str_to_uuid(uuid.string())];
        }

        [m_impl->central_manager scanForPeripheralsWithServices:services options:nil];
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Central::stop_scanning()
    {
        [m_impl->central_manager stopScan];
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Central::connect_peripheral(const PeripheralPtr &p, PeripheralCallback cb)
    {
        // connect peripheral
        if(p->connectable())
        {
            LOG_DEBUG << "connecting: " << p->name();
            auto it = g_peripheral_map.find(p);
            if(it != g_peripheral_map.end())
            {
                if(cb){ m_impl->peripheral_connected_cb = cb; }
                [m_impl->central_manager connectPeripheral:it->second options:nil];
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Central::disconnect_peripheral(const PeripheralPtr &the_peripheral)
    {
        LOG_DEBUG << "disconnecting: " << the_peripheral->name();
        auto it = g_peripheral_map.find(the_peripheral);

        if(it != g_peripheral_map.end())
        {
            CBPeripheral *cb_peripheral = it->second;

            [m_impl->central_manager cancelPeripheralConnection:cb_peripheral];
            g_peripheral_map.erase(the_peripheral);
            g_peripheral_reverse_map.erase(cb_peripheral);
            [cb_peripheral release];
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Central::disconnect_all()
    {
        for(auto &it : g_peripheral_map)
        {
            CBPeripheral *cb_peripheral = it.second;

            [m_impl->central_manager cancelPeripheralConnection:cb_peripheral];
            [cb_peripheral release];
        }
        g_peripheral_map.clear();
        g_peripheral_reverse_map.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    std::set<PeripheralPtr> Central::peripherals() const
    {
        std::set<PeripheralPtr> ret;
        for(const auto &pair : g_peripheral_map){ ret.insert(pair.first); }
        return ret;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Central::set_peripheral_discovered_cb(PeripheralCallback cb)
    {
        m_impl->peripheral_discovered_cb = cb;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Central::set_peripheral_connected_cb(PeripheralCallback cb)
    {
        m_impl->peripheral_connected_cb = cb;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Central::set_peripheral_disconnected_cb(PeripheralCallback cb)
    {
        m_impl->peripheral_disconnected_cb = cb;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////

    Peripheral::Peripheral():m_impl(new PeripheralImpl()){}

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    const std::string& Peripheral::name() const{ return m_impl->name; }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    bool Peripheral::is_connected() const
    {
        // gather our services
        NSMutableArray<CBUUID *> *services = [NSMutableArray<CBUUID *> array];
        for(const auto &pair : m_impl->known_services)
        {
            [services addObject:kinski::bluetooth::str_to_uuid(pair.first.string())];
        }
        
        CBCentralManager * m = m_impl->m_central_impl_ref.lock()->central_manager;
        NSArray<CBPeripheral*>*
        connected_peripherals = [m retrieveConnectedPeripheralsWithServices:services];
        
        PeripheralPtr self_ptr = std::const_pointer_cast<Peripheral>(shared_from_this());
        auto it = g_peripheral_map.find(self_ptr);
        
        if(it != g_peripheral_map.end())
        {
            CBPeripheral* p = it->second;
            if([connected_peripherals containsObject:p]){ return true; }
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    bool Peripheral::connectable() const { return m_impl->connectable; }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    int Peripheral::rssi() const { return m_impl->rssi; }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    const std::string Peripheral::identifier() const
    {
        PeripheralPtr self_ptr = std::const_pointer_cast<Peripheral>(shared_from_this());
        auto it = g_peripheral_map.find(self_ptr);
        
        if(it != g_peripheral_map.end())
        {
            CBPeripheral* p = it->second;
            return [[[p identifier] UUIDString] UTF8String];
        }
        return "";
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    CentralPtr Peripheral::central() const
    {
        CentralPtr ret;
        auto central_impl = m_impl->m_central_impl_ref.lock();
        if(central_impl){ ret = central_impl->central_ref.lock(); }
        return ret;
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Peripheral::discover_services(const std::set<UUID>& the_uuids)
    {
        PeripheralPtr self_ptr = shared_from_this();

        auto it = g_peripheral_map.find(self_ptr);

        if(it != g_peripheral_map.end())
        {
            CBPeripheral* p = it->second;

            // scan for services
            NSMutableArray<CBUUID *> *services = [NSMutableArray<CBUUID *> array];
            for(const auto &uuid : the_uuids)
            {
                [services addObject:kinski::bluetooth::str_to_uuid(uuid.string())];
            }

            [p discoverServices:services];
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Peripheral::write_value_for_characteristic(const UUID &the_characteristic,
                                                    const std::vector<uint8_t> &the_data)
    {
        CBCharacteristic *cb_characteristic = find_characteristic(shared_from_this(),
                                                                  the_characteristic);
        CBPeripheral* p = find_peripheral(shared_from_this());

        if(p && cb_characteristic)
        {
            const size_t max_packet_size = 20;
            size_t offset = 0;

            while(offset < the_data.size())
            {
                uint32_t num_bytes = std::min(max_packet_size, the_data.size() - offset);
                NSData *ns_data = [NSData dataWithBytes:(void*)&the_data[offset] length: num_bytes];
                [p writeValue: ns_data forCharacteristic: cb_characteristic type: CBCharacteristicWriteWithResponse];
                offset += num_bytes;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Peripheral::read_value_for_characteristic(const UUID &the_characteristic,
                                                   Peripheral::ValueUpdatedCallback cb)
    {
        CBCharacteristic *cb_characteristic = find_characteristic(shared_from_this(),
                                                                  the_characteristic);
        CBPeripheral* p = find_peripheral(shared_from_this());

        if(p && cb_characteristic)
        {
            [p readValueForCharacteristic:cb_characteristic];
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Peripheral::add_service(const UUID& the_service_uuid)
    {
        auto it = m_impl->known_services.find(the_service_uuid);

        if(it == m_impl->known_services.end())
        {
            m_impl->known_services[the_service_uuid] = {};
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Peripheral::add_characteristic(const UUID& the_service_uuid,
                                        const UUID& the_characteristic_uuid)
    {
        add_service(the_service_uuid);
        m_impl->known_services[the_service_uuid].insert(the_characteristic_uuid);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    const std::map<UUID, std::set<UUID>>& Peripheral::known_services()
    {
        return m_impl->known_services;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    Peripheral::ValueUpdatedCallback Peripheral::value_updated_cb(){ return m_impl->value_updated_cb; }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void Peripheral::set_value_updated_cb(ValueUpdatedCallback cb){ m_impl->value_updated_cb = cb; }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    CBPeripheral* find_peripheral(PeripheralPtr the_peripheral)
    {
        auto it = g_peripheral_map.find(the_peripheral);

        if(it != g_peripheral_map.end())
        {
            return it->second;
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    
    CBCharacteristic* find_characteristic(PeripheralPtr the_peripheral,
                                          const UUID& the_characteristic_uuid)
    {
        CBPeripheral* p = find_peripheral(the_peripheral);

        if(p)
        {
            // search for CB characteristic
            for(CBService *service in p.services)
            {
                for(CBCharacteristic *c in service.characteristics)
                {
                    auto characteristic_uuid =
                    kinski::bluetooth::UUID([c.UUID.UUIDString UTF8String]);

                    if(characteristic_uuid == the_characteristic_uuid){ return c;}
                }
            }
        }
        return nullptr;
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
}}//namespace

@implementation CoreBluetoothDelegate

///////////////////////////////////////////////////////////////////////////////////////////

- (instancetype) initWithImpl: (kinski::bluetooth::CentralImpl*) the_impl
{
    self.central_impl = the_impl;
    return [self init];
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManagerDidUpdateState:(CBCentralManager *)central
{

}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager *)central willRestoreState:(NSDictionary *)dict
{}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager *)central didRetrievePeripherals:(NSArray *)peripherals
{
    for(CBPeripheral* p in peripherals)
    {
        LOG_DEBUG << "retrieved: " << [p.name UTF8String];
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager *)central didRetrieveConnectedPeripherals:(NSArray *)peripherals
{}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager *)central
        didDiscoverPeripheral:(CBPeripheral *)peripheral
        advertisementData:(NSDictionary *)advertisementData
        RSSI:(NSNumber *)RSSI
{
    auto p = self.central_impl->create_peripheral();

    self.central_impl->set_peripheral_name(p, peripheral.name ? [peripheral.name UTF8String] : "unknown");
    NSString *local_name = [advertisementData objectForKey:CBAdvertisementDataLocalNameKey];
    
    if(local_name)
    {
        self.central_impl->set_peripheral_name(p, [local_name UTF8String]);
    }
    self.central_impl->set_peripheral_rssi(p, RSSI.floatValue);
    self.central_impl->set_peripheral_connectable(p, [[advertisementData objectForKey:CBAdvertisementDataIsConnectable] boolValue]);

    peripheral.delegate = self;
    kinski::bluetooth::g_peripheral_map[p] = [peripheral retain];
    kinski::bluetooth::g_peripheral_reverse_map[peripheral] = p;

    if(self.central_impl->peripheral_discovered_cb)
    {
        self.central_impl->peripheral_discovered_cb(self.central_impl->central_ref.lock(), p);
    }

    if(!p->connectable())
    {
        kinski::bluetooth::g_peripheral_map.erase(p);
        kinski::bluetooth::g_peripheral_reverse_map.erase(peripheral);
        [peripheral release];
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);

    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        LOG_DEBUG << "connected: " << p->name() << " (" << p->rssi() << ")";

        if(self.central_impl->peripheral_connected_cb)
        {
            self.central_impl->peripheral_connected_cb(self.central_impl->central_ref.lock(), p);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);

    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        LOG_WARNING << "failed to connect: " << p->name();
        [peripheral release];
        kinski::bluetooth::g_peripheral_map.erase(p);
        kinski::bluetooth::g_peripheral_reverse_map.erase(peripheral);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager *)central
didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);

    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;

        [peripheral release];
        kinski::bluetooth::g_peripheral_map.erase(p);
        kinski::bluetooth::g_peripheral_reverse_map.erase(peripheral);
        LOG_DEBUG << "disconnected: " << p->name();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheralDidUpdateRSSI:(CBPeripheral *)peripheral error:(nullable NSError *)error
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);

    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        self.central_impl->set_peripheral_rssi(p, [[peripheral RSSI] floatValue]);
        LOG_TRACE_2 << p->name() << ": " << kinski::to_string(p->rssi(), 1);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(nullable NSError *)error
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);

    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;

        for (CBService *service in peripheral.services)
        {
            auto service_uuid = kinski::bluetooth::UUID([service.UUID.UUIDString UTF8String]);
            LOG_TRACE_1 << "discovered service: " << service_uuid.string();

            auto service_it = p->known_services().find(service_uuid);

            if(service_it == p->known_services().end())
            {
                p->add_service(service_uuid);

                // now look for the characteristics for this service
                [peripheral discoverCharacteristics:nil forService:service];
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheralDidUpdateName:(CBPeripheral *)peripheral
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);

    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        self.central_impl->set_peripheral_name(p, [[peripheral name] UTF8String]);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral *)peripheral
        didDiscoverCharacteristicsForService:(CBService *)service
        error:(nullable NSError *)error
{
    auto service_uuid = kinski::bluetooth::UUID([service.UUID.UUIDString UTF8String]);

    LOG_TRACE_1 << "discovered " << service.characteristics.count << " characteristics for service: "
        << service_uuid.string();


    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);

    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;

        for (CBCharacteristic *c in service.characteristics)
        {
            auto characteristic_uuid = kinski::bluetooth::UUID([c.UUID.UUIDString UTF8String]);
            p->add_characteristic(service_uuid, characteristic_uuid);

            if([c properties] & CBCharacteristicPropertyNotify)
            {            
                LOG_TRACE_1 << "subscribed to characteristic: " << characteristic_uuid.string();
                [peripheral setNotifyValue:YES forCharacteristic: c];
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral *)peripheral
        didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
        error:(nullable NSError *)error
{
    LOG_TRACE_2 << "characteristic updated: " << (char*)[characteristic.value bytes];

    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);

    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        auto characteristic_uuid = kinski::bluetooth::UUID([characteristic.UUID.UUIDString UTF8String]);
        std::vector<uint8_t> value_vec((uint8_t*)[characteristic.value bytes],
                                       (uint8_t*)[characteristic.value bytes] + [characteristic.value length]);

        if(p->value_updated_cb()){ p->value_updated_cb()(characteristic_uuid, value_vec); }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral *)peripheral
        didWriteValueForCharacteristic:(CBCharacteristic *)characteristic
        error:(nullable NSError *)error
{
    if(error)
    {
        LOG_WARNING << "Error writing characteristic value: " << [error localizedDescription].UTF8String;
    }
}

@end

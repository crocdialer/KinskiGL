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

namespace kinski{ namespace bluetooth{
    
    typedef std::map<PeripheralPtr, CBPeripheral*> PeripheralMap;
    typedef std::map<CBPeripheral*, PeripheralPtr> PeripheralReverseMap;
    
    PeripheralMap g_peripheral_map;
    PeripheralReverseMap g_peripheral_reverse_map;
    
    std::string uuid_to_str(CBUUID *the_uuid)
    {
        return [the_uuid.UUIDString UTF8String];
    }
    
    CBUUID* str_to_uuid(const std::string &the_str)
    {
        return [CBUUID UUIDWithString:[NSString stringWithUTF8String:the_str.c_str()]];
    }
    
    UUID::UUID()
    {
        for(int i = 0; i < 16; i++){ m_data[i] = kinski::random_int<uint8_t>(0, 255); }
    }
    
    UUID::UUID(const std::string &the_str)
    {
        // remove "-"s
        auto str = the_str.substr(0, 8);
        str += the_str.substr(9, 4);
        str += the_str.substr(14, 4);
        str += the_str.substr(19, 4);
        str += the_str.substr(24, 12);
        
        for(int i = 0; i < 16; i++){ m_data[i] = std::stoul(str.substr(i * 2, 2), 0 , 16); }
    }
    
    UUID::UUID(uint8_t the_bytes[16])
    {
        memcpy(m_data, the_bytes, 16);
    }
    
    const uint8_t* UUID::as_bytes() const{ return m_data; }
    
    const std::string UUID::as_string() const
    {
        std::stringstream ss;
        
        for(int i = 0; i < 16; i++)
        {
            ss << std::setfill('0') << std::setw(2) << std::hex << (int)m_data[i];
        }
        auto ret = ss.str();
        ret.insert(8, "-");
        ret.insert(13, "-");
        ret.insert(18, "-");
        ret.insert(23, "-");
        return ret;
    }
    
    bool UUID::operator==(const UUID &the_other) const
    {
        for(int i = 0; i < 16; i++){ if(m_data[i] != the_other.m_data[i]){ return false; } }
        return true;
    }
    
    bool UUID::operator!=(const UUID &the_other) const { return !(*this == the_other); }
    
    bool UUID::operator<(const UUID &the_other) const
    {
        for(int i = 0; i < 16; i++)
        {
            if(m_data[i] == the_other.m_data[i]){ continue; }
            if(m_data[i] < the_other.m_data[i]){ return true; }
        }
        return false;
    }
    
    struct CentralImpl
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
    };
    
    CentralPtr Central::create(){ return CentralPtr(new Central()); }
    
    Central::Central():
    m_impl(new CentralImpl){ }
    
    void Central::discover_peripherals(std::set<UUID> the_service_uuids)
    {
        LOG_DEBUG << "discover_peripherals";
        m_impl->central_ref = shared_from_this();
        NSMutableArray<CBUUID *> *services = [NSMutableArray<CBUUID *> array];
        
        for(const auto &uuid : the_service_uuids)
        {
            [services addObject:kinski::bluetooth::str_to_uuid(uuid.as_string())];
        }
        
        [m_impl->central_manager scanForPeripheralsWithServices:services options:nil];
    }
    
    void Central::stop_scanning()
    {
        [m_impl->central_manager stopScan];
    }
    
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
    
    std::set<PeripheralPtr> Central::peripherals() const
    {
        std::set<PeripheralPtr> ret;
        for(const auto &pair : g_peripheral_map){ ret.insert(pair.first); }
        return ret;
    }
    
    void Central::set_peripheral_discovered_cb(PeripheralCallback cb)
    {
        m_impl->peripheral_discovered_cb = cb;
    }
    
    void Central::set_peripheral_connected_cb(PeripheralCallback cb)
    {
        m_impl->peripheral_connected_cb = cb;
    }
    
    void Central::set_peripheral_disconnected_cb(PeripheralCallback cb)
    {
        m_impl->peripheral_disconnected_cb = cb;
    }
    
///////////////////////////////////////////////////////////////////////////////////////////
    
    struct PeripheralImpl
    {
        std::weak_ptr<Central> central_ref;
        
        char uuid[16];
        std::string name = "unknown";
        bool connectable = false;
        bool connected = false;
        float rssi;
    };
    
    PeripheralPtr Peripheral::create(CentralPtr the_central, uint8_t *the_uuid)
    {
        auto ret = PeripheralPtr(new Peripheral);
        memcpy(ret->m_impl->uuid, the_uuid, 16);
        ret->m_impl->central_ref = the_central;
        return ret;
    }
    
    Peripheral::Peripheral():m_impl(new PeripheralImpl()){}
    
    std::string Peripheral::uuid() const{ return m_impl->uuid; }
    
    std::string Peripheral::name() const{ return m_impl->name; }
    
    void Peripheral::set_name(const std::string &the_name){ m_impl->name = the_name; }
    
    bool Peripheral::is_connected() const{ return m_impl->connected; }
    void Peripheral::set_connected(bool b){ m_impl->connected = b; }
    
    bool Peripheral::connectable() const { return m_impl->connectable; }
    
    void Peripheral::set_connectable(bool b){ m_impl->connectable = b; }
    
    float Peripheral::rssi() const { return m_impl->rssi; }
    
    void Peripheral::set_rssi(float the_rssi){ m_impl->rssi = the_rssi; }
    
    void Peripheral::discover_services(std::set<UUID> the_uuids)
    {
        if(!is_connected())
        {
            LOG_WARNING << "could not discover services, peripheral not connected";
            return;
        }
        
        PeripheralPtr self_ptr = shared_from_this();
        CentralPtr central_ptr = m_impl->central_ref.lock();
        
        if(!central_ptr){ return; }
        
        auto it = g_peripheral_map.find(self_ptr);
        
        if(it != g_peripheral_map.end())
        {
            CBPeripheral* p = it->second;

            // scan for services
            NSMutableArray<CBUUID *> *services = [NSMutableArray<CBUUID *> array];
            for(const auto &uuid : the_uuids)
            {
                [services addObject:kinski::bluetooth::str_to_uuid(uuid.as_string())];
            }
            
            [p discoverServices:services];
        }
    }
    
}}//namespace

@implementation CoreBluetoothDelegate

- (instancetype) initWithImpl: (kinski::bluetooth::CentralImpl*) the_impl
{
    self.central_impl = the_impl;
    return [self init];;
}

- (void)centralManagerDidUpdateState:(CBCentralManager *)central
{
    
}

- (void)centralManager:(CBCentralManager *)central willRestoreState:(NSDictionary *)dict
{}

- (void)centralManager:(CBCentralManager *)central didRetrievePeripherals:(NSArray *)peripherals
{
    for(CBPeripheral* p in peripherals)
    {
        LOG_DEBUG << "retrieved: " << [p.name UTF8String];
    }
}

- (void)centralManager:(CBCentralManager *)central didRetrieveConnectedPeripherals:(NSArray *)peripherals
{}

- (void)centralManager:(CBCentralManager *)central
        didDiscoverPeripheral:(CBPeripheral *)peripheral
        advertisementData:(NSDictionary *)advertisementData
        RSSI:(NSNumber *)RSSI
{
    auto p = kinski::bluetooth::Peripheral::create(self.central_impl->central_ref.lock(),
                                                   (uint8_t*)peripheral.identifier);
    
    p->set_name(peripheral.name ? [peripheral.name UTF8String] : "unknown");
    NSString *local_name = [advertisementData objectForKey:CBAdvertisementDataLocalNameKey];
    if(local_name){ p->set_name([local_name UTF8String]); }
    p->set_rssi(RSSI.floatValue);
    p->set_connectable([[advertisementData objectForKey:CBAdvertisementDataIsConnectable] boolValue]);
    
    
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

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);
    
    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        LOG_DEBUG << "connected: " << p->name() << " (" << kinski::as_string(p->rssi(), 1) << ")";
        p->set_connected(true);
        
        if(self.central_impl->peripheral_connected_cb)
        {
            self.central_impl->peripheral_connected_cb(self.central_impl->central_ref.lock(), p);
        }
    }
}

- (void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);
    
    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        LOG_DEBUG << "failed to connect: " << p->name();
        [peripheral release];
        kinski::bluetooth::g_peripheral_map.erase(p);
        kinski::bluetooth::g_peripheral_reverse_map.erase(peripheral);
    }
}

- (void)centralManager:(CBCentralManager *)central
didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);
    
    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        p->set_connected(false);
        LOG_DEBUG << "disconnected: " << p->name();
        
        [peripheral release];
        kinski::bluetooth::g_peripheral_map.erase(p);
        kinski::bluetooth::g_peripheral_reverse_map.erase(peripheral);
    }
}

- (void)peripheralDidUpdateRSSI:(CBPeripheral *)peripheral error:(nullable NSError *)error
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);
    
    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        p->set_rssi([[peripheral RSSI] floatValue]);
        LOG_DEBUG << p->name() << ": " << kinski::as_string(p->rssi(), 1);
    }
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(nullable NSError *)error
{
    for (CBService *service in peripheral.services)
    {
        LOG_DEBUG << "discovered service: " << [service.description UTF8String];
        [peripheral discoverCharacteristics:nil forService:service];
    }
}

- (void)peripheralDidUpdateName:(CBPeripheral *)peripheral
{
    auto it = kinski::bluetooth::g_peripheral_reverse_map.find(peripheral);
    
    if(it != kinski::bluetooth::g_peripheral_reverse_map.end())
    {
        auto p = it->second;
        p->set_name([[peripheral name] UTF8String]);
    }
}

- (void)peripheral:(CBPeripheral *)peripheral
        didDiscoverCharacteristicsForService:(CBService *)service
        error:(nullable NSError *)error
{
    LOG_DEBUG << "discovered characteristics for service: " << [service.description UTF8String];
    
    for (CBCharacteristic *c in service.characteristics)
    {
        LOG_DEBUG << "subscribed to characteristic: " << [c.description UTF8String];
        [peripheral setNotifyValue:YES forCharacteristic: c];
    }
}

- (void)peripheral:(CBPeripheral *)peripheral
        didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
        error:(nullable NSError *)error
{
    LOG_DEBUG << "characteristic updated: " << (char*)[characteristic.value bytes];
}

@end

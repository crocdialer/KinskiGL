//
//  bluetooth.cpp
//
//  Created by Fabian on 3/5/15.
//
//

#include "bluetooth.hpp"
#include "CoreBluetooth/CoreBluetooth.h"

@interface CentralManagerDelegate : NSObject<CBCentralManagerDelegate, CBPeripheralDelegate>
{
    kinski::bluetooth::CentralImpl *m_central_impl;
}
- (instancetype) initWithImpl: (kinski::bluetooth::CentralImpl*) the_impl;

@property(assign) kinski::bluetooth::CentralImpl *central_impl;
@end

namespace kinski{ namespace bluetooth{
    
    typedef std::map<PeripheralPtr, CBPeripheral*> PeripheralMap;
    typedef std::map<CBPeripheral*, PeripheralPtr> PeripheralReverseMap;
    
    struct CentralImpl
    {
        CBCentralManager* central_manager;
        
        PeripheralMap m_peripheral_map;
        PeripheralReverseMap m_peripheral_reverse_map;
        
        CentralManagerDelegate* delegate;
        
        PeripheralCallback
        peripheral_discovered_cb, peripheral_connected_cb, peripheral_disconnected_cb;
        
        std::weak_ptr<Central> central_ref;
        
        CentralImpl()
        {
            delegate = [[CentralManagerDelegate alloc] initWithImpl:this];
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
    
    void Central::scan_for_peripherals()
    {
        LOG_DEBUG << "scan_for_peripherals";
        m_impl->central_ref = shared_from_this();
        [m_impl->central_manager scanForPeripheralsWithServices:nil options:nil];
    }
    
    void Central::connect_peripheral(const PeripheralPtr &p, PeripheralCallback cb)
    {
        // connect peripheral
        if(p->is_connectable())
        {
            LOG_DEBUG << "connecting: " << p->name();
            auto it = m_impl->m_peripheral_map.find(p);
            if(it != m_impl->m_peripheral_map.end())
            {
                m_impl->peripheral_connected_cb = cb;
                [m_impl->central_manager connectPeripheral:it->second options:nil];
            }
        }
    }
    
    void Central::disconnect_peripheral(const PeripheralPtr &the_peripheral)
    {
        LOG_DEBUG << "disconnecting: " << the_peripheral->name();
        auto it = m_impl->m_peripheral_map.find(the_peripheral);
        
        if(it != m_impl->m_peripheral_map.end())
        {
            CBPeripheral *cb_peripheral = it->second;
            
            [m_impl->central_manager cancelPeripheralConnection:cb_peripheral];
            m_impl->m_peripheral_map.erase(the_peripheral);
            m_impl->m_peripheral_reverse_map.erase(cb_peripheral);
            [cb_peripheral release];
        }
    }
    
    std::set<PeripheralPtr> Central::peripherals() const
    {
        std::set<PeripheralPtr> ret;
        for(const auto &pair : m_impl->m_peripheral_map){ ret.insert(pair.first); }
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
        char uuid[16];
        std::string name = "unknown";
        bool is_connectable = false;
        float rssi;
    };
    
    PeripheralPtr Peripheral::create(uint8_t *the_uuid)
    {
        auto ret = PeripheralPtr(new Peripheral);
        memcpy(ret->m_impl->uuid, the_uuid, 16);
        return ret;
    }
    
    Peripheral::Peripheral():m_impl(new PeripheralImpl()){}
    
    std::string Peripheral::uuid() const{ return m_impl->uuid; }
    
    std::string Peripheral::name() const{ return m_impl->name; }
    
    void Peripheral::set_name(const std::string &the_name){ m_impl->name = the_name; }
    
    bool Peripheral::is_connectable() const { return m_impl->is_connectable; }
    
    void Peripheral::set_connectable(bool b){ m_impl->is_connectable = b; }
    
    float Peripheral::rssi() const { return m_impl->rssi; }
    
    void Peripheral::set_rssi(float the_rssi){ m_impl->rssi = the_rssi; }
    
}}//namespace

@implementation CentralManagerDelegate

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

- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral
     advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI
{
    auto p = kinski::bluetooth::Peripheral::create((uint8_t*)peripheral.identifier);
    
    p->set_name(peripheral.name ? [peripheral.name UTF8String] : "unknown");
    NSString *local_name = [advertisementData objectForKey:CBAdvertisementDataLocalNameKey];
    if(local_name){ p->set_name([local_name UTF8String]); }
    p->set_rssi(RSSI.floatValue);
    p->set_connectable([[advertisementData objectForKey:CBAdvertisementDataIsConnectable] boolValue]);
    
    
    peripheral.delegate = self;
    self.central_impl->m_peripheral_map[p] = [peripheral retain];
    self.central_impl->m_peripheral_reverse_map[peripheral] = p;
    
    if(self.central_impl->peripheral_discovered_cb)
    {
        self.central_impl->peripheral_discovered_cb(self.central_impl->central_ref.lock(), p);
    }
    
    if(!p->is_connectable())
    {
        self.central_impl->m_peripheral_map.erase(p);
        self.central_impl->m_peripheral_reverse_map.erase(peripheral);
        [peripheral release];
    }
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
    auto it = self.central_impl->m_peripheral_reverse_map.find(peripheral);
    
    if(it != self.central_impl->m_peripheral_reverse_map.end())
    {
        auto p = it->second;
        LOG_DEBUG << "connected: " << p->name();
        
        if(self.central_impl->peripheral_connected_cb)
        {
            self.central_impl->peripheral_connected_cb(self.central_impl->central_ref.lock(), p);
        }
    }
    
}

- (void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    auto it = self.central_impl->m_peripheral_reverse_map.find(peripheral);
    
    if(it != self.central_impl->m_peripheral_reverse_map.end())
    {
        auto p = it->second;
        LOG_DEBUG << "failed to connect: " << p->name();
        [peripheral release];
        self.central_impl->m_peripheral_map.erase(p);
        self.central_impl->m_peripheral_reverse_map.erase(peripheral);
    }
}

- (void)centralManager:(CBCentralManager *)central
didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    auto it = self.central_impl->m_peripheral_reverse_map.find(peripheral);
    
    if(it != self.central_impl->m_peripheral_reverse_map.end())
    {
        auto p = it->second;
        LOG_DEBUG << "disconnected: " << p->name();
        [peripheral release];
        self.central_impl->m_peripheral_map.erase(p);
        self.central_impl->m_peripheral_reverse_map.erase(peripheral);
    }
}

- (void)peripheralDidUpdateRSSI:(CBPeripheral *)peripheral error:(nullable NSError *)error
{
    auto it = self.central_impl->m_peripheral_reverse_map.find(peripheral);
    
    if(it != self.central_impl->m_peripheral_reverse_map.end())
    {
        auto p = it->second;
        p->set_rssi([[peripheral RSSI] floatValue]);
        LOG_DEBUG << p->name() << ": " << kinski::as_string(p->rssi(), 1);
    }
}

@end

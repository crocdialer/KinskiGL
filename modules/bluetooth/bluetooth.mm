//
//  bluetooth.cpp
//
//  Created by Fabian on 3/5/15.
//
//

#include "bluetooth.hpp"
#include "CoreBluetooth/CoreBluetooth.h"

@interface CentralManagerDelegate : NSObject<CBCentralManagerDelegate>
{
    kinski::bluetooth::CentralImpl *m_central_impl;
}
- (instancetype) initWithImpl: (kinski::bluetooth::CentralImpl*) the_impl;

@property(assign) kinski::bluetooth::CentralImpl *central_impl;
@end

namespace kinski{ namespace bluetooth{
    
    typedef std::map<PeripheralPtr, CBPeripheral*> PeripheralMap;
    
    struct CentralImpl
    {
        CBCentralManager* central_manager;
        PeripheralMap m_peripheral_map;
        CentralManagerDelegate* delegate;
        PeripheralDiscoveredCallback peripheral_discovered_cb;
        PeripheralConnectedCallback peripheral_connected_cb;
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
    
    void Central::connect_peripheral(const PeripheralPtr &p)
    {
        // connect peripheral
        if(p->is_connectable)
        {
            LOG_DEBUG << "connecting: " << p->name;
            auto it = m_impl->m_peripheral_map.find(p);
            if(it != m_impl->m_peripheral_map.end())
            {
                [m_impl->central_manager connectPeripheral:it->second options:nil];
            }
        }
    }
    
    std::set<PeripheralPtr> Central::peripherals() const
    {
        std::set<PeripheralPtr> ret;
        for(const auto &pair : m_impl->m_peripheral_map){ ret.insert(pair.first); }
        return ret;
    }
    
    void Central::set_peripheral_discovered_cb(PeripheralDiscoveredCallback cb)
    {
        m_impl->peripheral_discovered_cb = cb;
    }
    
    void Central::set_peripheral_connected_cb(PeripheralConnectedCallback cb)
    {
        m_impl->peripheral_connected_cb = cb;
    }
    
///////////////////////////////////////////////////////////////////////////////////////////
    
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
    auto p = std::make_shared<kinski::bluetooth::Peripheral>();
    
    p->name = peripheral.name ? [peripheral.name UTF8String] : "unknown";
    NSString *local_name = [advertisementData objectForKey:CBAdvertisementDataLocalNameKey];
    if(local_name){ p->name = [local_name UTF8String]; }
    p->is_connectable = [[advertisementData objectForKey:CBAdvertisementDataIsConnectable] boolValue];
    memcpy(&p->uuid, peripheral.identifier, 16);
    
    self.central_impl->m_peripheral_map[p] = [peripheral retain];
    
    if(self.central_impl->peripheral_discovered_cb)
    {
        self.central_impl->peripheral_discovered_cb(self.central_impl->central_ref.lock(), p,
                                                    RSSI.floatValue);
    }
    
    if(!p->is_connectable)
    {
        self.central_impl->m_peripheral_map.erase(p);
        [peripheral release];
    }
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
    auto it = std::find_if(self.central_impl->m_peripheral_map.begin(),
                           self.central_impl->m_peripheral_map.end(),
                           [peripheral](kinski::bluetooth::PeripheralMap::value_type pair)
    {
        return pair.second == peripheral;
    });
    
    if(it != self.central_impl->m_peripheral_map.end())
    {
        auto p = it->first;
        LOG_DEBUG << "connected: " << p->name;
        
        if(self.central_impl->peripheral_connected_cb)
        {
            self.central_impl->peripheral_connected_cb(self.central_impl->central_ref.lock(), p);
        }
    }
    
}

- (void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    auto it = std::find_if(self.central_impl->m_peripheral_map.begin(),
                           self.central_impl->m_peripheral_map.end(),
                           [peripheral](kinski::bluetooth::PeripheralMap::value_type pair)
    {
        return pair.second == peripheral;
    });
    
    if(it != self.central_impl->m_peripheral_map.end())
    {
        auto p = it->first;
        LOG_DEBUG << "failed to connect: " << p->name;
        [it->second release];
        self.central_impl->m_peripheral_map.erase(p);
    }
}

- (void)centralManager:(CBCentralManager *)central
didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    auto it = std::find_if(self.central_impl->m_peripheral_map.begin(),
                           self.central_impl->m_peripheral_map.end(),
                           [peripheral](kinski::bluetooth::PeripheralMap::value_type pair)
    {
        return pair.second == peripheral;
    });
    
    if(it != self.central_impl->m_peripheral_map.end())
    {
        auto p = it->first;
        LOG_DEBUG << "disconnected: " << p->name;
        [it->second release];
        self.central_impl->m_peripheral_map.erase(p);
    }
}

@end

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

    struct CentralImpl
    {
        CBCentralManager* central_manager;
        CentralManagerDelegate* delegate;
        PeripheralDiscoveredCallback peripheral_discovered_cb;
        Central *central = nullptr;
        
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
    
    Central::Central():
    m_impl(new CentralImpl)
    {
        m_impl->central = this;
    }
    
    void Central::scan_for_peripherals()
    {
        LOG_DEBUG << "scan_for_peripherals";
        [m_impl->central_manager scanForPeripheralsWithServices:nil options:nil];
    }
    
    void Central::connect_peripheral(const Peripheral &the_peripheral)
    {
    
    }
    
    void Central::set_peripheral_discovered_cb(PeripheralDiscoveredCallback cb)
    {
        m_impl->peripheral_discovered_cb = cb;
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
    kinski::bluetooth::Peripheral p;
    
    p.name = peripheral.name ? [peripheral.name UTF8String] : "unknown";
    NSString *local_name = [advertisementData objectForKey:CBAdvertisementDataLocalNameKey];
    if(local_name){ p.name = [local_name UTF8String]; }
    bool is_connectable = [[advertisementData objectForKey:CBAdvertisementDataIsConnectable] boolValue];
    
    memcpy(&p.uuid, peripheral.identifier, 16);
    
    if(self.central_impl->peripheral_discovered_cb)
    {
        self.central_impl->peripheral_discovered_cb(*self.central_impl->central, p, p.uuid,
                                                    RSSI.floatValue);
    }
    
    // connect peripheral
    if(is_connectable){ [central connectPeripheral:peripheral options:nil]; }
    else{ LOG_DEBUG << "not connectable";}
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
    LOG_DEBUG << "connected: " << peripheral.name ? [peripheral.name UTF8String] : "unknown";
}

- (void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{
    LOG_DEBUG << "failed to connect: " << [peripheral.name UTF8String];
}

- (void)centralManager:(CBCentralManager *)central
didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error
{}

@end

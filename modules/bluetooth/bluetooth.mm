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
}
@end

@implementation CentralManagerDelegate
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
    LOG_DEBUG << "discovered: " << [peripheral.name UTF8String];
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral
{
    LOG_DEBUG << "connected: " << [peripheral.name UTF8String];
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

namespace kinski{ namespace bluetooth{

    struct Central::CentralImpl
    {
        CBCentralManager* central_manager;
        CentralManagerDelegate* delegate;
        CentralImpl()
        {
            delegate = [[CentralManagerDelegate alloc] init];
            central_manager = [[CBCentralManager alloc] initWithDelegate:delegate queue:nil options:nil];
        }
        ~CentralImpl()
        {
            [central_manager dealloc];
            [delegate dealloc];
        }
    };
    
    Central::Central():
    m_impl(new CentralImpl)
    {
        
    }
    
    void Central::scan_for_peripherals()
    {
        LOG_DEBUG << "scan_for_peripherals";
        [m_impl->central_manager scanForPeripheralsWithServices:nil options:nil];
    }
    
    void Central::connect_peripheral(const Peripheral &the_peripheral)
    {
    
    }
    
///////////////////////////////////////////////////////////////////////////////////////////
    
}}//namespace

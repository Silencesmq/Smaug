# Embedded Encrypted Database SQLite in TrustZone

## Prerequsite
PC: Ubuntu 18.04
Board: STM32MP157C-DK2

## Developer Environment Configure
SEE "Developer Environment Configure.pdf".

### STM32MP1 Developer Environment Configure
1. PC Prerequisites
2. Installing the Starter Package
3. Installing the SDK
4. Installing the Linux kernel
5. Installing the OP-TEE
6. Installing the OPTEE-CLIENT

### OPTEE CA & TA Developer Environment Configure
1. CA Developer Environment Configure
2. TA Developer Environment Configure

## Smaug-tpm
SEE "Smaug-tpm.pdf".

### Modifying the Linux kernel
1. Modifying the Linux kernel device tree
2. Configure the Linux kernel Menuconfig

### Modifying the OP-TEE
1. Add System Call
2. Add System Service
3. Updating the OP-TEE

### Modifying the OPTEE-CLIENT
1. Add RPC Service
2. Updating the OPTEE-CLIENT

## Smaug-sqlite
SEE "Smaug-sqlite.pdf".
And CA/TA in Directory "Smaug".

### Modifying the OP-TEE
1. Add System Call
2. Add System Service
3. Updating the OP-TEE

### Modifying the OPTEE-CLIENT
1. Add RPC Service
2. Updating the OPTEE-CLIENT

## TrustSQLite
SEE "TrustSQLite.pdf".
And CA/TA in Directory "TrustSQLite".

### Modifying the OP-TEE
1. Add System Call
2. Add System Service
3. Updating the OP-TEE

### Modifying the OPTEE-CLIENT
1. Add RPC Service
2. Updating the OPTEE-CLIENT

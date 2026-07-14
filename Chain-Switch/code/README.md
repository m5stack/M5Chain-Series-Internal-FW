# Storage Structure Description
## 1. FLASH Structure
| Start Address | End Address | Region Name | Size | Function Description |
| -------------- | ------------ | ------------ | ----- | -------------------- |
| 0x08000000 | 0x08001FFF | Boot | 8 KB | Stores the bootloader and firmware upgrade logic |
| 0x08002000 | 0x0800DFFF | App | 48 KB | User application program storage area |
| 0x0800E000 | 0x0800FFFF | Data | 8 KB | Stores system parameters |
---
## 2. RAM Structure
**Note:** Boot and App share the same RAM area and occupy their corresponding spaces during different runtime stages.
| Start Address | End Address | Region Name | Size | Function Description |
| -------------- | ------------ | ------------ | ----- | -------------------- |
| 0x20000000 | 0x20001FFF | Boot | 8192 bytes | Used during the boot stage for buffering and variable storage |
| 0x200000C0 | 0x20001FFF | App | 8000 bytes | Used during application runtime for task stacks, variables, and caching |
---
## 3. Detailed Description
- **Boot Region (8 KB)**  
  Used to store the bootloader and firmware upgrade control logic.  
  The last byte stores the firmware version number for version management and upgrade validation.  
- **App Region (48 KB)**  
  Used to store the user application program.  
  The last 4 bytes are reserved for CRC checksum storage. The bootloader verifies the program before loading.  
  The verification uses the **standard CRC-32 algorithm** (IEEE 802.3, polynomial `0x04C11DB7`, initial value `0xFFFFFFFF`, input/output reflected, result bitwise inverted, and stored in little-endian byte order).  
- **Data Region (8 KB)**  
  Used to store system parameters.
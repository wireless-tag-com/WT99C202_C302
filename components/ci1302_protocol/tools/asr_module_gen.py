#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import struct
from typing import Dict, Any

# partition_single_t: <I I I I B (4+4+4+4+1=17) packed, 3字节对齐补齐到20？但C里没补齐，直接17字节，后面紧跟下一个结构体
# partition_table_t: 结构体定义见 ci1302_ota_uart.h

# CRC16-CCITT查找表，与C代码中的table_crc16_ccitt保持一致
TABLE_CRC16_CCITT = [
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
]


def crc16_ccitt(pre_crc: int, data: bytes) -> int:
    """
    计算给定数据的16位CRC校验值, 数据格式为CCITT标准(初始值0x0000，低位在前，高位在后，结果与0x0000异或)。
    
    参数:
        pre_crc: 前一次计算的CRC值，如果是第一批数据，此值为0
        data: 需要计算CRC校验值的数据（bytes类型）
    
    返回:
        int: 计算出来的16位CRC校验值
        
    示例:
        >>> # 计算单个数据块的CRC
        >>> data = b"Hello, World!"
        >>> crc = crc16_ccitt(0, data)
        >>> print(f"CRC16-CCITT: 0x{crc:04X}")
        
        >>> # 分块计算CRC（流式处理）
        >>> crc = 0
        >>> crc = crc16_ccitt(crc, b"Hello, ")
        >>> crc = crc16_ccitt(crc, b"World!")
        >>> print(f"CRC16-CCITT: 0x{crc:04X}")
    """
    if not isinstance(data, (bytes, bytearray)):
        raise TypeError("数据必须是bytes或bytearray类型")
    
    if not isinstance(pre_crc, int) or pre_crc < 0 or pre_crc > 0xFFFF:
        raise ValueError("pre_crc必须是0-65535之间的整数")
    
    ret = pre_crc & 0xFFFF  # 确保是16位
    
    for byte in data:
        # 与C代码中的算法保持一致:
        # ret = (ret << 8) ^ table_crc16_ccitt[((ret >> 8) ^ *data) & 0x00FF];
        index = ((ret >> 8) ^ byte) & 0xFF
        ret = ((ret << 8) ^ TABLE_CRC16_CCITT[index]) & 0xFFFF
    
    return ret

def parse_partition_single(data: bytes, offset: int) -> (Dict[str, Any], int):
    """
    解析 partition_single_t 结构体
    """
    fmt = '<4IB'
    size = struct.calcsize(fmt)
    version, addr, size_, crc, status = struct.unpack_from(fmt, data, offset)
    return {
        'version': version,
        'addr': addr,
        'size': size_,
        'crc': crc,
        'status': status
    }, offset + size


def parse_partition_table(data: bytes) -> Dict[str, Any]:
    """
    解析 pat.bin 的前 4096 字节为 partition_table_t 字典
    """
    offset = 0
    d = {}
    d['manu_facturer_id'], = struct.unpack_from('<I', data, offset)
    offset += 4
    d['product_id'] = struct.unpack_from('<2I', data, offset)
    offset += 8
    d['hard_ware_name'] = struct.unpack_from('<16I', data, offset)
    offset += 64
    d['hard_ware_version'], = struct.unpack_from('<I', data, offset)
    offset += 4
    d['soft_ware_name'] = struct.unpack_from('<16I', data, offset)
    offset += 64
    d['soft_ware_version'], = struct.unpack_from('<I', data, offset)
    offset += 4
    d['bootloader_version'], = struct.unpack_from('<I', data, offset)
    offset += 4
    d['ChipName'] = data[offset:offset+9].decode(errors='ignore').rstrip('\0')
    offset += 9
    d['FirmwareFormatVer'] = data[offset]
    offset += 1
    d['reserve'] = data[offset:offset+4]
    offset += 4
    # partition_single_t 共6个
    for key in ['user_code1', 'user_code2', 'asr_cmd_mode', 'dnn_model', 'voice', 'user_file']:
        d[key], offset = parse_partition_single(data, offset)
    d['nv_data_offset'], = struct.unpack_from('<I', data, offset)
    offset += 4
    d['nv_data_size'], = struct.unpack_from('<I', data, offset)
    offset += 4
    d['partition_table_checksum'], = struct.unpack_from('<H', data, offset)
    offset += 2
    return d

def pack_partition_single(partition: Dict[str, Any]) -> bytes:
    """
    将 partition_single_t 字典打包为二进制数据
    """
    return struct.pack('<4IB', 
                      partition['version'],
                      partition['addr'], 
                      partition['size'],
                      partition['crc'],
                      partition['status'])


def pack_partition_table(d: Dict[str, Any]) -> bytes:
    """
    将 partition_table_t 字典重新打包为4096字节的二进制数据
    会重新计算 partition_table_checksum
    """
    data = bytearray()
    
    # manu_facturer_id (4 bytes)
    data.extend(struct.pack('<I', d['manu_facturer_id']))
    
    # product_id (8 bytes)
    data.extend(struct.pack('<2I', *d['product_id']))
    
    # hard_ware_name (64 bytes)
    data.extend(struct.pack('<16I', *d['hard_ware_name']))
    
    # hard_ware_version (4 bytes)
    data.extend(struct.pack('<I', d['hard_ware_version']))
    
    # soft_ware_name (64 bytes)
    data.extend(struct.pack('<16I', *d['soft_ware_name']))
    
    # soft_ware_version (4 bytes)
    data.extend(struct.pack('<I', d['soft_ware_version']))
    
    # bootloader_version (4 bytes)
    data.extend(struct.pack('<I', d['bootloader_version']))
    
    # ChipName (9 bytes)
    chip_name = d['ChipName'].encode('utf-8')[:9]
    chip_name += b'\0' * (9 - len(chip_name))  # 填充到9字节
    data.extend(chip_name)
    
    # FirmwareFormatVer (1 byte)
    data.extend(struct.pack('<B', d['FirmwareFormatVer']))
    
    # reserve (4 bytes)
    data.extend(d['reserve'])
    
    # partition_single_t 共6个 (每个17字节)
    for key in ['user_code1', 'user_code2', 'asr_cmd_mode', 'dnn_model', 'voice', 'user_file']:
        data.extend(pack_partition_single(d[key]))
    
    # nv_data_offset (4 bytes)
    data.extend(struct.pack('<I', d['nv_data_offset']))
    
    # nv_data_size (4 bytes)
    data.extend(struct.pack('<I', d['nv_data_size']))
    
    # 计算校验和 - 对前面所有数据求和（不包括校验和本身）
    checksum = sum(data) & 0xFFFF
    
    # partition_table_checksum (2 bytes)
    data.extend(struct.pack('<H', checksum))
    
    # 填充到4096字节，不足的部分用0xFF填充
    if len(data) < 4096:
        data.extend(b'\xFF' * (4096 - len(data)))
    elif len(data) > 4096:
        # 如果超过4096字节，截断到4096字节
        data = data[:4096]
    
    return bytes(data)


def print_partition_table(d):
    print(f"pat->hard_ware_name: {d['hard_ware_version'] >> 24}.{(d['hard_ware_version'] >> 16) & 0xFF}.{d['hard_ware_version'] & 0xFFFF}")
    print(f"pat->soft_ware_version: {d['soft_ware_version'] >> 24}.{(d['soft_ware_version'] >> 16) & 0xFF}.{d['soft_ware_version'] & 0xFFFF}")
    for key in ['user_code1', 'user_code2', 'asr_cmd_mode', 'dnn_model', 'voice', 'user_file']:
        v = d[key]
        print(f"pat->{key}.version: {v['version'] >> 24}.{(v['version'] >> 16) & 0xFF}.{v['version'] & 0xFFFF}, size: {v['size']}, crc: 0x{v['crc']:04x}")

def main():
    script_dir =  os.path.dirname(os.path.abspath(__file__))
    files_dir = os.path.join(script_dir, "..", "asr_ota_file")

    firmware_path = os.path.join(files_dir, "ci1302_firmware.bin")
    asr_bin_path = os.path.join(files_dir, "asr.bin")
    user_file_path = os.path.join(files_dir, "user_file.bin")
    output_path = os.path.join(files_dir, "asr_by_gen.bin")

    with open(firmware_path, 'rb') as f:
        firmware = f.read()

    # 解析分区表
    partition_table = parse_partition_table(firmware[0x6000:])
    print("原始分区表:")
    print_partition_table(partition_table)
    print(f"原始校验和: 0x{partition_table['partition_table_checksum']:04x}")
    
    asr_data = open(asr_bin_path, 'rb').read()
    partition_table["asr_cmd_mode"]["size"] = len(asr_data)
    partition_table["asr_cmd_mode"]["crc"] = crc16_ccitt(0, asr_data)
    asr_data = asr_data + b'\xFF' * (4096 - len(asr_data) % 4096)

    user_file_data = open(user_file_path, 'rb').read()
    partition_table["user_file"]["size"] = len(user_file_data)
    partition_table["user_file"]["crc"] = crc16_ccitt(0, user_file_data)

    # 重新打包分区表
    repacked_data = pack_partition_table(partition_table)
    # 验证重新打包的数据
    repacked_partition_table = parse_partition_table(repacked_data)
    print("\n重新打包后的分区表:")
    print_partition_table(repacked_partition_table)
    print(f"新校验和: 0x{repacked_partition_table['partition_table_checksum']:04x}")

    # partition, asr model offset
    repacked_data = repacked_data + firmware[0x6000+4096:0x6000 + 4096 * 2] + asr_data + user_file_data

    # 保存重新打包的数据
    with open(output_path, 'wb') as f:
        f.write(repacked_data)

    print(f"\n重新打包的分区表已保存到: {output_path}")

if __name__ == "__main__":
    main()

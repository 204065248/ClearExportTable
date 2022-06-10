// Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#include <vector>
#include <atlstr.h>
#include <fstream>
#include <iostream>

using namespace std;

// 导出表项
typedef struct tagExportItem {
	DWORD m_dwIndex;
	DWORD m_dwOrder;
	DWORD m_dwRva;
	CString m_strFuncName;
} EXPORT_ITEM, * PEXPORT_ITEM;

// 导出表详情
typedef struct tagExportInfo {
	CString m_strExportName;
	std::vector<EXPORT_ITEM> m_exportItems;
} EXPORT_INFO, * PEXPORT_INFO;

// 内存偏移转文件偏移
int Rva2Raw(PIMAGE_SECTION_HEADER pSection, int nSectionNum, int nRva) {
	int nRet = 0;
	// 遍历节区
	for (int i = 0; i < nSectionNum; i++) {
		// 导出表地址在这个节区内
		if (pSection[i].VirtualAddress <= nRva && nRva < pSection[i + 1].VirtualAddress) {
			// 文件偏移 = 该段的 PointerToRawData + (内存偏移 - 该段起始的RVA(VirtualAddress))
			nRet = nRva - pSection[i].VirtualAddress + pSection[i].PointerToRawData;
			break;
		}
	}
	return nRet;
}

// 32位清空导出表
BOOL ClearExpTable32(CString strFilePath, EXPORT_INFO& exportInfo) {
	// 二进制方式读文件
	fstream file(strFilePath, ios::binary | ios::in);
	if (!file) {
		return FALSE;
	}

	// 读取全部
	file.seekg(0, ios::end);
	int nFileSize = file.tellg();
	file.seekg(0, ios::beg);
	char* pBuffer = new char[nFileSize];
	file.read(pBuffer, nFileSize);
	file.close();

	// 读 dos 头
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBuffer;
	// 读 nt 头
	PIMAGE_NT_HEADERS32 pNtHeader = (PIMAGE_NT_HEADERS32)(pBuffer + pDosHeader->e_lfanew);
	// 判定是否有导出表
	if (!pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress) {
		delete[] pBuffer;
		return TRUE;
	}

	// 读节区头
	PIMAGE_SECTION_HEADER pSection = (PIMAGE_SECTION_HEADER)(pBuffer + pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS32));
	// 读导出表
	int nSectionNum = pNtHeader->FileHeader.NumberOfSections;
	PIMAGE_EXPORT_DIRECTORY pExpDir = (PIMAGE_EXPORT_DIRECTORY)(pBuffer + Rva2Raw(pSection, nSectionNum, pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress));
	// 读导出表头
	char* lpszExportName = (char*)(pBuffer + Rva2Raw(pSection, nSectionNum, pExpDir->Name));
	exportInfo.m_strExportName = lpszExportName;

	// 获取到处函数个数
	int nAddressNum = pExpDir->NumberOfFunctions;
	// 获取导出表函数名称表
	int* lpFuncNameAddr = (int*)(pBuffer + Rva2Raw(pSection, nSectionNum, pExpDir->AddressOfNames));
	// 获取导出表函数序号表
	short* lphOrder = (short*)(pBuffer + Rva2Raw(pSection, nSectionNum, pExpDir->AddressOfNameOrdinals));
	// 获取导出表函数地址表
	int* lpFuncAddr = (int*)(pBuffer + Rva2Raw(pSection, nSectionNum, pExpDir->AddressOfFunctions));

	// 遍历导出表，并清空内容
	for (int i = 0; i < nAddressNum; i++) {

		EXPORT_ITEM item{};
		item.m_dwIndex = i;
		item.m_dwOrder = lphOrder[i];
		item.m_dwRva = lpFuncAddr[i];
		item.m_strFuncName = "no name";

		// 判定是否有函数名
		if (lpFuncNameAddr[i] > 0) {
			char* lpszFuncName = (char*)(pBuffer + Rva2Raw(pSection, nSectionNum, lpFuncNameAddr[i]));
			item.m_strFuncName = lpszFuncName;
			// 清空函数名
			memset(lpszFuncName, 0, strlen(lpszFuncName));
		}
		// 清空ID
		lphOrder[i] = 0;
		// 清空RVA
		lpFuncAddr[i] = 0;

		// 加入信息中
		exportInfo.m_exportItems.push_back(item);
	}

	// 清空导出表
	memset(pExpDir, 0, sizeof(IMAGE_EXPORT_DIRECTORY));
	// 清空索引
	pNtHeader->OptionalHeader.DataDirectory[0].Size = 0;
	pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress = 0;

	// 写出到文件
	file.open(strFilePath, ios::binary | ios::out);
	if (!file) {
		delete[] pBuffer;
		return FALSE;
	}
	file.write(pBuffer, nFileSize);
	file.close();
	delete[] pBuffer;
	return TRUE;

}

// 64位清空导出表
BOOL ClearExpTable64(CString strFilePath, EXPORT_INFO& exportInfo) {
	// 二进制方式读文件
	fstream file(strFilePath, ios::binary | ios::in);
	if (!file) {
		return FALSE;
	}

	// 读取全部
	file.seekg(0, ios::end);
	int nFileSize = file.tellg();
	file.seekg(0, ios::beg);
	char* pBuffer = new char[nFileSize];
	file.read(pBuffer, nFileSize);
	file.close();

	// 读 dos 头
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBuffer;
	// 读 nt 头
	PIMAGE_NT_HEADERS64 pNtHeader = (PIMAGE_NT_HEADERS64)(pBuffer + pDosHeader->e_lfanew);
	// 判定是否有导出表
	if (!pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress) {
		delete[] pBuffer;
		return TRUE;
	}

	// 读节区头
	PIMAGE_SECTION_HEADER pSection = (PIMAGE_SECTION_HEADER)(pBuffer + pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS64));
	// 读导出表
	int nSectionNum = pNtHeader->FileHeader.NumberOfSections;
	PIMAGE_EXPORT_DIRECTORY pExpDir = (PIMAGE_EXPORT_DIRECTORY)(pBuffer + Rva2Raw(pSection, nSectionNum, pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress));
	// 读导出表头
	char* lpszExportName = (char*)(pBuffer + Rva2Raw(pSection, nSectionNum, pExpDir->Name));
	exportInfo.m_strExportName = lpszExportName;

	// 获取到处函数个数
	int nAddressNum = pExpDir->NumberOfFunctions;
	// 获取导出表函数名称表
	int* lpFuncNameAddr = (int*)(pBuffer + Rva2Raw(pSection, nSectionNum, pExpDir->AddressOfNames));
	// 获取导出表函数序号表
	short* lphOrder = (short*)(pBuffer + Rva2Raw(pSection, nSectionNum, pExpDir->AddressOfNameOrdinals));
	// 获取导出表函数地址表
	int* lpFuncAddr = (int*)(pBuffer + Rva2Raw(pSection, nSectionNum, pExpDir->AddressOfFunctions));

	// 遍历导出表，并清空内容
	for (int i = 0; i < nAddressNum; i++) {

		EXPORT_ITEM item{};
		item.m_dwIndex = i;
		item.m_dwOrder = lphOrder[i];
		item.m_dwRva = lpFuncAddr[i];
		item.m_strFuncName = "no name";

		// 判定是否有函数名
		if (lpFuncNameAddr[i] > 0) {
			char* lpszFuncName = (char*)(pBuffer + Rva2Raw(pSection, nSectionNum, lpFuncNameAddr[i]));
			item.m_strFuncName = lpszFuncName;
			// 清空函数名
			memset(lpszFuncName, 0, strlen(lpszFuncName));
		}
		// 清空ID
		lphOrder[i] = 0;
		// 清空RVA
		lpFuncAddr[i] = 0;

		// 加入信息中
		exportInfo.m_exportItems.push_back(item);
	}

	// 清空导出表
	memset(pExpDir, 0, sizeof(IMAGE_EXPORT_DIRECTORY));
	// 清空索引
	pNtHeader->OptionalHeader.DataDirectory[0].Size = 0;
	pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress = 0;

	// 写出到文件
	file.open(strFilePath, ios::binary | ios::out);
	if (!file) {
		delete[] pBuffer;
		return FALSE;
	}
	file.write(pBuffer, nFileSize);
	file.close();
	delete[] pBuffer;
	return TRUE;

}

BOOL Is64BitFile(CString strFilePath) {
	// 二进制方式读文件
	fstream file(strFilePath, ios::binary | ios::in);
	if (!file) {
		return FALSE;
	}

	// 读取全部
	file.seekg(0, ios::end);
	int nFileSize = file.tellg();
	file.seekg(0, ios::beg);
	char* pBuffer = new char[nFileSize];
	file.read(pBuffer, nFileSize);
	file.close();

	// 读 dos 头
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBuffer;
	// 读 nt 头
	PIMAGE_NT_HEADERS64 pNtHeader = (PIMAGE_NT_HEADERS64)(pBuffer + pDosHeader->e_lfanew);
	// 判断是否64位
	if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		delete[] pBuffer;
		return TRUE;
	}
	delete[] pBuffer;
	return FALSE;
}

int main(int argc, char* argv[], char* envp[]) {
	// 获取命令行path参数
	CString strFilePath = argv[1];
	// 判断文件是32位还是64位
	BOOL bIs64 = Is64BitFile(strFilePath);

	std::cout << "程序路径：" << strFilePath << std::endl;
	std::cout << "当前程序为：" << (bIs64 ? 64 : 32) << "位程序" << std::endl;
	std::cout << "开始清空导出表..." << std::endl;
	EXPORT_INFO exportInfo;
	if (bIs64) {
		if (!ClearExpTable64(strFilePath, exportInfo)) {
			std::cout << "导出表清空失败" << std::endl;
			return 0;
		}
	} else {
		if (!ClearExpTable32(strFilePath, exportInfo)) {
			std::cout << "导出表清空失败" << std::endl;
			return 0;
		}
	}

	std::cout << "原导出表内容" << std::endl;
	std::cout << "Export Name: " << exportInfo.m_strExportName << std::endl;
	for (auto& item : exportInfo.m_exportItems) {
		std::cout << "ID: [" << item.m_dwIndex << "]\t"
			<< "Order: [" << item.m_dwOrder << "]\t"
			<< "Rva: [0x" << std::hex << item.m_dwRva << "]\t"
			<< "Name: [" << item.m_strFuncName << "]\t" << std::endl;
	}

	system("pause");
	return 0;
}

#include "context.h"
#include "mft.h"
#include "helpers.h"

bool SetUppercaseList(execution_context context);

execution_context SetupContext(int argc, char* argv[])
{
	SafeCreate(result, execution_context);
	memset(result, 0, sizeof(struct _execution_context));

	if (!((result->parameters = Parse(argc, argv))))
		return ErrorCleanUp(free, result, "");

	if (!(result->boot = ReadFromDisk(result->parameters->source_drive, result->parameters->image_offs)))
		return ErrorCleanUp((void (*)(void*))CleanUp, result, "");

	result->cluster_sz = (uint32_t)(result->boot->bytes_per_sector * result->boot->sectors_per_cluster);

	int8_t cpr = result->boot->clusters_per_file_record;
	if (cpr < 0)
	{
		// If negative, size is 2 to the power of the absolute value
		result->mft_record_sz = (1UL << -cpr);
	}
	else
	{
		// If positive, size is in clusters
		result->mft_record_sz = result->cluster_sz * cpr;
	}

	if (result->parameters->write_boot_info)
	{
		string buffer = StringPrint(NULL, 0, L"%ls\\VolInfo.txt", BaseString(result->parameters->output_folder));
		FILE* f;
		if (!(_wfopen_s(&f, BaseString(buffer), L"w+")))
		{
			PrintInformation(result->boot, f);
			if (f) fclose(f);
		}
		DeleteString(buffer);
	}

	if (!(result->dr = OpenDiskReader(result->parameters->source_drive, result->boot->bytes_per_sector)))
		return ErrorCleanUp((void (*)(void*))CleanUp, result, "");

	if (!(result->mft_table = LoadMFTFile(result, MASTER_FILE_TABLE_NUMBER)))
		return ErrorCleanUp((void (*)(void*))CleanUp, result, "");

	if (!SetUppercaseList(result))
		return ErrorCleanUp((void (*)(void*))CleanUp, result, "");

	return result;
}

bool SetUppercaseList(execution_context context)
{
	mft_file file = LoadMFTFile(context, UPCASE_TABLE_NUMBER);
	if (!file)
		return false;

	attribute lst = FirstAttribute(context, file, AttrTypeFlag(ATTR_DATA));

	bool result = lst && AttributeSize(lst) == 0x20000;
	if (result)
	{
		bytes table = GetBytesFromAttrib(context, file, lst, 0, 0x20000);
		context->upper_case = (wchar_t*)table->buffer;
		free(table);
	}
	else
		printf("Error: UpCase file incomplete.\n");

	DeleteMFTFile(file);

	return result;
}

void CleanUp(void* context_ptr)
{
    execution_context context = (execution_context)context_ptr;
	if (context->boot)
		free(context->boot);

	if (context->parameters)
		DeleteSettings(context->parameters);

	if (context->mft_table)
		DeleteMFTFile(context->mft_table);

	if (context->dr)
		CloseDiskReader(context->dr);

	if (context->writer)
		CloseDataWriter(context->writer);

	if (context->upper_case)
		free(context->upper_case);

	free(context);
}

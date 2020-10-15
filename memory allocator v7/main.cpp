#include <stdio.h>
#include <stdint.h>
#define MEM_SIZE 1024


static char virtual_memory[MEM_SIZE];
static void* START;
static size_t SIZE = 812;

#pragma pack(push, 1)
typedef struct header
{
	bool state; // 0 == free; 1 == busy
	size_t prev_size;
	size_t size;
} header_t;
#pragma pack(pop)
static int HEADER_SIZE = sizeof(header_t);

size_t get_prev_size(void* ptr)
{
	return ((header_t*)ptr)->prev_size;
}
size_t get_size(void* ptr)
{
	return ((header_t*)ptr)->size;
}
bool get_state(void* ptr)
{
	return ((header_t*)ptr)->state;
}

void set_prev_size(void* ptr, size_t prev_size)
{
	((header_t*)ptr)->prev_size = prev_size;
}
void set_size(void* ptr, size_t size)
{
	((header_t*)ptr)->size = size;
}
void set_state(void* ptr, bool state)
{
	((header_t*)ptr)->state = state;
}

void move_next(void* ptr)
{
	if ((uint8_t*)ptr + get_size(ptr) + HEADER_SIZE == (uint8_t*)START + SIZE + HEADER_SIZE)
	{
		ptr = NULL;
	}
	else {
		ptr = (uint8_t*)ptr + HEADER_SIZE + get_size(ptr);
	}
}
void* get_next(void* ptr)
{
	if ((uint8_t*)ptr + get_size(ptr) + HEADER_SIZE == (uint8_t*)START + SIZE + HEADER_SIZE)
	{
		return NULL;
	}
	return (uint8_t*)ptr + HEADER_SIZE + get_size(ptr);
}

void move_prev(void* ptr)
{
	if (ptr == START)
	{
		ptr = NULL;
	}
	else {
		ptr = (uint8_t*)ptr - HEADER_SIZE - get_prev_size(ptr);
	}
}
void* get_prev(void* ptr)
{
	if (ptr == START)
	{
		return NULL;
	}
	return (uint8_t*)ptr - HEADER_SIZE - get_prev_size(ptr);
}

void build_header(void* ptr, bool state, size_t prev_size, size_t size)
{
	header_t temp;
	temp.prev_size = prev_size;
	temp.size = size;
	temp.state = state;
	*((header_t*)ptr) = temp;
}



void unit_headers(void* ptr1, void* ptr2)
{
	set_size(ptr1, get_size(ptr1) + get_size(ptr2) + HEADER_SIZE);
	if (get_next(ptr2) != nullptr)
	{
		set_prev_size(get_next(ptr1), get_size(ptr2));
	}
}

void* get_free_block(size_t size)
{
	//Алгоритм управления памятью TLSF
	//Появился перевод описания алгоритма управления памятью TLSF. 
	//Данный алгоритм характерен тем, что эффективность его O(1) и использует он стратегию выделения памяти "хорошо подходящими" (good-fit) блоками. 
	void* cursor = START;
	void* best = nullptr;
	while (cursor != nullptr)
	{
		if (best == nullptr && get_state(cursor) == 0 && get_size(cursor) >= size)
		{
			best = cursor;
		}
		if (get_state(cursor) == 0 && get_size(cursor) >= size && get_size(cursor) < get_size(best))
		{
			best = cursor;
		}
		cursor = get_next(cursor);
	}
	return best;
}

void mem_dump()
{
	void* pointer = START;
	size_t size_h = 0;
	size_t size_b = 0;
	printf("%c", 218);
	for (int i = 0; i < 51; i++)
	{
		if (i % 17 == 0 && i > 0)
		{
			printf("%c", 194);
		}
		printf("%c", 196);
	}
	for (int i = 0; i < 27; i++)
	{
		if (i % 9 == 0)
		{
			printf("%c", 194);
		}
		printf("%c", 196);
	}
	printf("%c\n", 191);
	printf("%c % 15s %c % 15s %c % 15s %c % 7s %c % 7s %c % 7s %c \n",
		179, "address", 179, "prev", 179, "next", 179, "state", 179, "size", 179, "pr_size", 179);
	while (pointer != NULL)
	{
		printf("%c % 15p %c % 15p %c % 15p %c % 7d %c % 7ld %c % 7ld %c \n",
			179, pointer, 179, get_prev(pointer), 179,
			get_next(pointer), 179, get_state(pointer), 179,
			get_size(pointer), 179, get_prev_size(pointer), 179);

		size_h = size_h + HEADER_SIZE;
		size_b = size_b + get_size(pointer);
		pointer = get_next(pointer);
	}

	printf("%c", 192);
	for (int i = 0; i < 51; i++)
	{
		if (i % 17 == 0 && i > 0)
		{
			printf("%c", 193);
		}
		printf("%c", 196);
	}
	for (int i = 0; i < 27; i++)
	{
		if (i % 9 == 0)
		{
			printf("%c", 193);
		}
		printf("%c", 196);
	}
	printf("%c\n", 217);
	printf("Mb of:\n\theaders: %ld\n\tblocks : %ld\n\tsummary : %ld\n", size_h, size_b, size_h + size_b);
}

void* block(size_t size)
{
	void* pointer = virtual_memory + HEADER_SIZE;
	build_header(pointer, false, 0, size);

	return pointer;
}
void mem_free(void* pointer)
{
	pointer = (uint8_t*)pointer - HEADER_SIZE;
	set_state(pointer, false);
	if (get_next(pointer) != nullptr && get_state(get_next(pointer)) == 0)
	{
		unit_headers(pointer, get_next(pointer));
	}
	if (get_prev(pointer) != nullptr && get_state(get_prev(pointer)) == 0)
	{
		unit_headers(get_prev(pointer), pointer);
	}
}

void* mem_alloc(size_t size)
{
	if (size % 4 != 0) //align 4 bytes
	{
		size = size - size % 4 + 4;
	}
	void* pointer = get_free_block(size);
	if (pointer == nullptr)
	{
		return pointer; // no enough memory
	}

	if (get_size(pointer) > size + HEADER_SIZE) // if can make it smallet
	{
		build_header((uint8_t*)pointer + size + HEADER_SIZE, 0, size, get_size(pointer) - size - HEADER_SIZE);
		set_size(pointer, size);
	}
	set_state(pointer, true);
	return (uint8_t*)pointer + HEADER_SIZE;
}

void* mem_realloc(void* pointer, size_t size)
{
	pointer = (uint8_t*)pointer - HEADER_SIZE;
	if (size % 4 != 0)
	{
		size = size - size % 4 + 4;
	}

	if (get_size(pointer) == size)
	{
		return pointer;
	}

	if (get_size(pointer) > size) // if block has more memory then it had
	{
		if (get_size(pointer) - size - HEADER_SIZE >= 0) // block size > header size
		{
			build_header((uint8_t*)pointer + size + HEADER_SIZE, false, size, get_size(pointer) - size -
				HEADER_SIZE);
			set_size(pointer, size);
			if (get_next(get_next(pointer)) != nullptr &&
				get_state(get_next(get_next(pointer))) == 0)
			{
				unit_headers(get_next(pointer), get_next(get_next(pointer)));
			}
		}
		return pointer;
	}

	if (get_next(pointer) != nullptr && get_size(pointer) + get_size(get_next(pointer)) >= size)
	{
		build_header((uint8_t*)pointer + size + HEADER_SIZE, false, size, get_size(get_next(pointer)) - (size - get_size(pointer)));
		set_size(pointer, size);
		return pointer;
	}

	if (get_next(pointer) != nullptr && get_state(get_next(pointer)) == 0 &&
		get_size(pointer) + get_size(get_next(pointer)) + HEADER_SIZE >= size)
	{
		set_size(pointer, get_size(pointer) + get_size(get_next(pointer)) + HEADER_SIZE);
		return pointer;
	}

	void* best = mem_alloc(size);
	if (best == nullptr)
	{
		return best;
	}
	mem_free((uint8_t*)pointer + HEADER_SIZE);
	return best;
}

int main()
{
	START = block(SIZE);
	mem_dump();
	void* x1 = mem_alloc(40);	//try allocate
	mem_dump();
	mem_realloc(x1, 120);		//try reallocate
	mem_dump();
	mem_free(x1);				//try free
	mem_dump();
	void* ptrs[3];				//try to fragmentate 
	for (int i = 0; i < 3; i++)
		ptrs[i] = mem_alloc(10);
	mem_dump();
	mem_free(ptrs[0]);
	mem_free(ptrs[2]);
	mem_dump();
	mem_free(ptrs[1]);
	mem_dump();
	void* inptrs[5];
	for (int i = 0; i < 5; i++)
		inptrs[i] = mem_alloc(10);
	mem_dump();
	mem_free(inptrs[1]);
	mem_free(inptrs[3]);
	mem_dump();
	mem_free(inptrs[2]);
	mem_dump();
	mem_free(inptrs[0]);
	mem_dump();
	return 0;
}
#include <datoviz.h>

#include <type_traits>

static_assert(std::is_same_v<decltype(&dvz_num_procs), int (*)()>);

int main()
{
    return 0;
}

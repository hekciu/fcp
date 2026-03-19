#include <stdlib.h>

#include "error_codes.h"
#include "common.h"


#define HANDLE_ERROR(int_val) {if ((int_val) != 0) { fcp_exit(int_val); }}




int main(int argc, char** argv) {
    fcp_print_help_message();

    return 0;
}

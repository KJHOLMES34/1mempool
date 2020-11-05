// Test Linear Fill

//TEST STORE_LOAD 4BYTE
Nop;
//BASE_ADDRESS = 32'h0000_0000;
//FILL_LINEAR ( .address_base(BASE_ADDRESS+COUNT_4B*4*SRC_ID),   .fill_pattern(32'hCAFE0000), .cmd_id(1), .transfer_count(COUNT_4B), .transfer_type("4_BYTE") );
//BARRIER_AW();
//CHECK_LINEAR  ( .address_base(BASE_ADDRESS+COUNT_4B*4*SRC_ID), .check_pattern(32'hCAFE0000), .cmd_id(2), .transfer_count(COUNT_4B), .transfer_type("4_BYTE") );
//BARRIER_AR();
Nop;

//TEST STORE_LOAD 8BYTE
BASE_ADDRESS = 32'h0000_0000;
FILL_LINEAR ( .address_base(BASE_ADDRESS+COUNT_8B*8*SRC_ID),   .fill_pattern(32'hC1A00000), .cmd_id(3), .transfer_count(COUNT_8B), .transfer_type("8_BYTE") );
BARRIER_AW();
CHECK_LINEAR  ( .address_base(BASE_ADDRESS+COUNT_8B*8*SRC_ID), .check_pattern(32'hC1A00000), .cmd_id(4), .transfer_count(COUNT_8B), .transfer_type("8_BYTE") );
BARRIER_AR();
Nop;


//TEST STORE_LOAD 16BYTE
Nop;
BASE_ADDRESS = 32'h0000_0000;
FILL_LINEAR ( .address_base(BASE_ADDRESS+COUNT_16B*16*SRC_ID),   .fill_pattern(32'hBAFE0000), .cmd_id(5), .transfer_count(COUNT_16B), .transfer_type("16_BYTE") );
BARRIER_AW();
CHECK_LINEAR  ( .address_base(BASE_ADDRESS+COUNT_16B*16*SRC_ID), .check_pattern(32'hBAFE0000), .cmd_id(6), .transfer_count(COUNT_16B), .transfer_type("16_BYTE") );
BARRIER_AR();
Nop;

//TEST STORE_LOAD 16BYTE
BASE_ADDRESS = 32'h0000_0000;
FILL_LINEAR ( .address_base(BASE_ADDRESS+COUNT_32B*32*SRC_ID),   .fill_pattern(32'hE1A00000), .cmd_id(7), .transfer_count(COUNT_32B), .transfer_type("32_BYTE") );
BARRIER_AW();
CHECK_LINEAR  ( .address_base(BASE_ADDRESS+COUNT_32B*32*SRC_ID), .check_pattern(32'hE1A00000), .cmd_id(8), .transfer_count(COUNT_32B), .transfer_type("32_BYTE") );
BARRIER_AR();
Nop;


BASE_ADDRESS = 32'h0000_0000;
FILL_RANDOM  ( .address_base(BASE_ADDRESS),   .fill_pattern(32'h00F10000), .cmd_id(9), .transfer_count(COUNT_32B), .transfer_type("32_BYTE") );
BARRIER_AW();

//TEST STORE_LOAD 8BYTE
BASE_ADDRESS = 32'h0000_1000;
FILL_LINEAR ( .address_base(BASE_ADDRESS+COUNT_8B*8*SRC_ID),   .fill_pattern(32'hC1A00000), .cmd_id(10), .transfer_count(COUNT_8B), .transfer_type("8_BYTE") );
BARRIER_AW();
CHECK_LINEAR  ( .address_base(BASE_ADDRESS+COUNT_8B*8*SRC_ID), .check_pattern(32'hC1A00000), .cmd_id(11), .transfer_count(COUNT_8B), .transfer_type("8_BYTE") );
BARRIER_AR();
Nop;
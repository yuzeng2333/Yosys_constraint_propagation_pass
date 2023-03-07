module test(
  input         clock,
  input         reset,
  input  [15:0] io_x,
  input  [15:0] io_y,
  input  [1:0]  io_opcode,
  output [15:0] io_result
);
  wire  _T = io_opcode == 2'h1; // @[GCD.scala 40:18]
  wire [15:0] _T_2 = io_x + io_y; // @[GCD.scala 41:23]
  wire  _T_3 = io_opcode == 2'h2; // @[GCD.scala 42:26]
  wire [15:0] _T_4 = io_x & io_y; // @[GCD.scala 43:23]
  wire  _T_5 = io_opcode == 2'h3; // @[GCD.scala 44:26]
  wire [15:0] _T_6 = io_x | io_y; // @[GCD.scala 45:23]
  wire [15:0] _T_8 = io_x - io_y; // @[GCD.scala 47:23]
  wire [15:0] _GEN_0 = _T_5 ? _T_6 : _T_8; // @[GCD.scala 44:35]
  wire [15:0] _GEN_1 = _T_3 ? _T_4 : _GEN_0; // @[GCD.scala 42:35]
  assign io_result = _T ? _T_2 : _GEN_1; // @[GCD.scala 41:15 GCD.scala 43:15 GCD.scala 45:15 GCD.scala 47:15]
endmodule

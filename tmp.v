// Benchmark "ex" written by ABC on Tue Dec 26 18:19:43 2023

module ex ( 
    a, b, c,
    F0, F1  );
  input  a, b, c;
  output F0, F1;
  assign F0 = a ? ~b : (b & ~c);
  assign F1 = ~b ^ c;
endmodule



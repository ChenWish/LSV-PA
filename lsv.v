// Benchmark "lsv/finalProject/google_ex00" written by ABC on Sun Dec 24 09:54:35 2023

module \lsv/finalProject/google_ex00  ( 
    pi00, pi01, pi02, pi03, pi04, pi05, pi06, pi07, pi08, pi09, pi10, pi11,
    pi12, pi13,
    po0, po1, po2, po3, po4, po5, po6  );
  input  pi00, pi01, pi02, pi03, pi04, pi05, pi06, pi07, pi08, pi09,
    pi10, pi11, pi12, pi13;
  output po0, po1, po2, po3, po4, po5, po6;
  wire new_n22, new_n23, new_n24, new_n25, new_n26, new_n27, new_n28,
    new_n29, new_n30, new_n31, new_n32, new_n33, new_n35, new_n36, new_n37,
    new_n38, new_n40, new_n41, new_n42, new_n43, new_n44, new_n45, new_n46,
    new_n47, new_n48, new_n49, new_n50, new_n51, new_n52, new_n53, new_n54,
    new_n55, new_n56, new_n57, new_n58, new_n59, new_n60, new_n61, new_n62,
    new_n63, new_n64, new_n65, new_n66, new_n67, new_n68, new_n69, new_n70,
    new_n71, new_n72, new_n73;
  assign new_n22 = ~pi04 & ~pi06;
  assign new_n23 = pi09 & new_n22;
  assign new_n24 = ~pi03 & pi12;
  assign new_n25 = new_n23 & ~new_n24;
  assign new_n26 = ~pi02 & pi07;
  assign new_n27 = pi04 & new_n22;
  assign new_n28 = pi03 & new_n27;
  assign new_n29 = pi02 & pi12;
  assign new_n30 = ~new_n27 & ~new_n29;
  assign new_n31 = ~pi07 & ~new_n30;
  assign new_n32 = ~pi03 & pi09;
  assign new_n33 = ~pi00 & new_n32;
  assign po2 = pi07 & ~pi12;
  assign new_n35 = ~pi13 & ~new_n26;
  assign new_n36 = ~po2 & ~new_n35;
  assign new_n37 = ~new_n33 & ~new_n36;
  assign new_n38 = ~new_n31 & ~new_n37;
  assign po6 = new_n28 | new_n38;
  assign new_n40 = ~new_n26 & po6;
  assign new_n41 = ~pi03 & ~new_n23;
  assign new_n42 = ~new_n30 & ~new_n41;
  assign new_n43 = ~new_n40 & ~new_n42;
  assign new_n44 = ~pi00 & ~new_n43;
  assign new_n45 = pi07 & new_n43;
  assign new_n46 = pi00 & new_n45;
  assign new_n47 = pi03 & ~pi07;
  assign new_n48 = ~new_n46 & ~new_n47;
  assign new_n49 = ~new_n44 & new_n48;
  assign new_n50 = ~new_n25 & new_n49;
  assign new_n51 = ~pi00 & ~new_n48;
  assign new_n52 = ~new_n46 & ~new_n51;
  assign new_n53 = pi01 & ~pi10;
  assign new_n54 = ~pi06 & new_n53;
  assign new_n55 = pi04 & ~new_n54;
  assign new_n56 = ~pi09 & new_n55;
  assign new_n57 = ~new_n52 & ~new_n56;
  assign new_n58 = ~new_n28 & ~new_n57;
  assign new_n59 = pi09 & ~new_n55;
  assign new_n60 = ~new_n27 & ~new_n59;
  assign new_n61 = ~new_n33 & ~new_n38;
  assign new_n62 = new_n22 & new_n53;
  assign new_n63 = ~pi02 & pi03;
  assign new_n64 = pi09 & new_n63;
  assign new_n65 = ~new_n62 & ~new_n64;
  assign new_n66 = ~new_n61 & new_n65;
  assign new_n67 = ~new_n60 & ~new_n66;
  assign new_n68 = ~new_n49 & new_n67;
  assign new_n69 = new_n29 & new_n43;
  assign new_n70 = ~pi00 & new_n40;
  assign new_n71 = ~new_n69 & ~new_n70;
  assign new_n72 = ~new_n68 & new_n71;
  assign new_n73 = ~new_n58 & ~new_n72;
  assign po1 = ~new_n50 & ~new_n73;
  assign po0 = ~pi05;
  assign po5 = ~pi08;
  assign po3 = pi11;
  assign po4 = pi13;
endmodule



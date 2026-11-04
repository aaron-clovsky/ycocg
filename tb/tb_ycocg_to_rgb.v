module tb_ycocg_to_rgb;

    logic clk;

    initial begin
        clk = 0;
    end

    always
        #5 clk = ~clk;

    reg [7:0]  y;
    reg [7:0]  co;
    reg [7:0]  cg;
    wire [7:0] r;
    wire [7:0] g;
    wire [7:0] b;

    ycocg_to_rgb test(.y(y),
                      .co(co),
                      .cg(cg),
                      .r(r),
                      .g(g),
                      .b(b));

    integer y_i, co_i, cg_i;

    initial begin
        for (y_i = 0; y_i <= 255; y_i++) begin
            for (co_i = 0; co_i <= 255; co_i++) begin
                for (cg_i = 0; cg_i <= 255; cg_i++) begin
                    y  = y_i[7:0];
                    co = co_i[7:0];
                    cg = cg_i[7:0];
                    @(posedge clk);
                    $display("%0d,%0d,%0d,%0d,%0d,%0d", y, co, cg, r, g, b);
                end
            end
        end

        $finish(0);
    end
endmodule

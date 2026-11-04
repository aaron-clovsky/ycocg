module tb_ycocg_to_rgb
    import ycocg_to_rgb::ycocg_to_rgb;;

    logic [7:0] r, g, b;

    initial begin
        for (int y = 0; y <= 255; y++) begin
            for (int co = 0; co <= 255; co++) begin
                for (int cg = 0; cg <= 255; cg++) begin
                    ycocg_to_rgb(y[7:0], co[7:0], cg[7:0], r, g, b);

                    $display("%0d,%0d,%0d,%0d,%0d,%0d", y, co, cg, r, g, b);
                end
            end
        end
        $finish(0);
    end
endmodule

package ycocg_to_rgb;

    function automatic[7:0] _clamp_u8;
        input signed [9:0] value;
        begin
            _clamp_u8 = value[9] ? 8'h00 : value[8] ? 8'hff : value[7:0];
        end
    endfunction

    function automatic void ycocg_to_rgb;
        input [7:0]  y, co, cg;
        output [7:0] r, g, b;
        begin
            logic signed [9:0] y_s  = 10'($signed({2'b0, y}));
            logic signed [9:0] co_s = 10'($signed(co));
            logic signed [9:0] cg_s = 10'($signed(cg));
            logic signed [9:0] tmp  = y_s - cg_s;

            r = _clamp_u8(tmp + co_s);
            g = _clamp_u8(y_s + cg_s);
            b = _clamp_u8(tmp - co_s);
        end
    endfunction
endpackage

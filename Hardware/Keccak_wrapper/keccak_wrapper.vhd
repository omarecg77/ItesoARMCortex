---MIT No Attribution

--Copyright (c) [2025] [Omar Castro]

--Permission is hereby granted, free of charge, to any person obtaining a copy
--of this software and associated documentation files (the "Software"), to deal
--in the Software without restriction, including without limitation the rights
--to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
--copies of the Software, and to permit persons to whom the Software is
--furnished to do so.

--THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
--IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
--FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
--AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
--LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
--OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
--SOFTWARE.

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
library work;
use work.keccak_globals.all;

entity keccak_round_wrapper is
    port (
        clk       : in  std_logic;
        reset     : in  std_logic;
        start     : in  std_logic;
        done      : out std_logic;
        init_0, init_1, init_2, init_3 : in  k_lane;
        init_4, init_5, init_6, init_7 : in  k_lane;
        init_8                    : in  k_lane;
        out_0,  out_1,  out_2,  out_3 : out k_lane;
        out_4,  out_5,  out_6,  out_7 : out k_lane
    );
end entity keccak_round_wrapper;

architecture behavior of keccak_round_wrapper is
    component keccak_round
        port (
            round_in              : in  k_state;
            round_constant_signal : in  std_logic_vector(63 downto 0);
            round_out             : out k_state
        );
    end component;

    component keccak_round_constants_gen
        port (
            round_number              : in  unsigned(4 downto 0);
            round_constant_signal_out : out std_logic_vector(63 downto 0)
        );
    end component;

    signal round_in_internal, round_out_internal : k_state;
    signal initial_state                         : k_state;
    signal round_constant_signal                 : std_logic_vector(63 downto 0);
    signal round_number                          : unsigned(4 downto 0) := (others => '0');
    signal processing                            : std_logic := '0';
    signal done_internal                         : std_logic := '0';

begin
    UUT_CONST: keccak_round_constants_gen
        port map (
            round_number              => round_number,
            round_constant_signal_out => round_constant_signal
        );

    UUT_ROUND: keccak_round
        port map (
            round_in              => round_in_internal,
            round_constant_signal => round_constant_signal,
            round_out             => round_out_internal
        );

    initial_state <= (
        (X"0000000000000000", X"0000000000000000", X"0000000000000000", X"0000000000000000", X"0000000000000000"),
        (X"0000000000000000", X"0000000000000000", X"0000000000000000", X"0000000000000000", X"0000000000000000"),
        (X"0000000000000000", X"0000000000000000", X"0000000000000000", X"0000000000000000", X"0000000000000000"),
        (X"0000000000000000", init_8, init_7, init_6, init_5),
        (init_4, init_3, init_2, init_1, init_0)
    );

    out_0 <= round_in_internal(0)(0);
    out_1 <= round_in_internal(0)(1);
    out_2 <= round_in_internal(0)(2);
    out_3 <= round_in_internal(0)(3);
    out_4 <= round_in_internal(0)(4);
    out_5 <= round_in_internal(1)(0);
    out_6 <= round_in_internal(1)(1);
    out_7 <= round_in_internal(1)(2);

    done <= done_internal;

    process(clk, reset)
        variable round_count : integer := 0;
        variable first_cycle : boolean := true;
        variable finished    : boolean := false;
    begin
        if reset = '1' then
            -- Reset state
            processing <= '0';
            done_internal <= '0';
            round_number <= (others => '0');
            round_count := 0;
            first_cycle := true;
            finished := false;
            round_in_internal <= (others => (others => (others => '0')));
        elsif rising_edge(clk) then
            if start = '1' and processing = '0' and finished = false then
                -- Start the process when start is asserted
                processing <= '1';
                round_count := 0;
                round_number <= (others => '0');
                round_in_internal <= initial_state;
                done_internal <= '0';
                first_cycle := false;
            elsif processing = '1' and not finished then
                if round_count < 23 then
                    round_in_internal <= round_out_internal;
                    round_number <= round_number + 1;
                    round_count := round_count + 1;
                elsif round_count = 23 then
                    round_in_internal <= round_out_internal;
                    round_count := round_count + 1;
                    finished := true;
                end if;
            elsif finished then
                processing <= '0';
                done_internal <= '1';
            end if;
        end if;
    end process;

end behavior;
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

entity keccak_round_wrapper_tb is
end entity keccak_round_wrapper_tb;

architecture behavior of keccak_round_wrapper_tb is
    component keccak_round_wrapper
        port (
            clk       : in  std_logic;
            reset     : in  std_logic;  -- Active-high reset
            start     : in  std_logic;
            done      : out std_logic;
            init_0, init_1, init_2, init_3 : in  k_lane;
            init_4, init_5, init_6, init_7 : in  k_lane;
            init_8                    : in  k_lane;
            out_0,  out_1,  out_2,  out_3 : out k_lane;
            out_4,  out_5,  out_6,  out_7 : out k_lane
        );
    end component;

    -- Testbench signals
    signal clk       : std_logic := '0';
    signal reset     : std_logic := '0';  -- Active-high reset
    signal start     : std_logic := '0';
    signal done      : std_logic;
    signal init_0, init_1, init_2, init_3 : k_lane;
    signal init_4, init_5, init_6, init_7 : k_lane;
    signal init_8                    : k_lane;
    signal out_0,  out_1,  out_2,  out_3 : k_lane;
    signal out_4,  out_5,  out_6,  out_7 : k_lane;

    constant CLK_PERIOD : time := 10 ns;

    -- Function to convert std_logic_vector to hex string
    function to_hex(slv : std_logic_vector) return string is
        constant HEX_CHARS : string(1 to 16) := "0123456789ABCDEF";
        variable result : string(1 to slv'length/4);
        variable nibble : std_logic_vector(3 downto 0);
    begin
        for i in 0 to slv'length/4 - 1 loop
            nibble := slv(i*4 + 3 downto i*4);
            result(result'length - i) := HEX_CHARS(to_integer(unsigned(nibble)) + 1);
        end loop;
        return result;
    end function to_hex;

begin
    -- Instantiate the DUT
    uut: keccak_round_wrapper
        port map (
            clk       => clk,
            reset     => reset,
            start     => start,
            done      => done,
            init_0    => init_0, init_1 => init_1, init_2 => init_2, init_3 => init_3,
            init_4    => init_4, init_5 => init_5, init_6 => init_6, init_7 => init_7,
            init_8    => init_8,
            out_0     => out_0,  out_1 => out_1,  out_2 => out_2,  out_3 => out_3,
            out_4     => out_4,  out_5 => out_5,  out_6 => out_6,  out_7 => out_7
        );

    -- Clock generation
    clk_process: process
    begin
        while true loop
            clk <= '0';
            wait for CLK_PERIOD/2;
            clk <= '1';
            wait for CLK_PERIOD/2;
        end loop;
    end process;

    -- Stimulus process
    stim_proc: process
    begin
        -- Initialize inputs
        init_0 <= X"00000000000006E5";
        init_1 <= X"0000000000000000";
        init_2 <= X"0000000000000000";
        init_3 <= X"0000000000000000";
        init_4 <= X"0000000000000000";
        init_5 <= X"0000000000000000";
        init_6 <= X"0000000000000000";
        init_7 <= X"0000000000000000";
        init_8 <= X"8000000000000000";

        -- Test Case 1: Reset and normal operation
        report "Test Case 1: Reset and normal operation" severity note;
        reset <= '1';
        wait for CLK_PERIOD * 2;  -- Hold reset for 2 cycles
        reset <= '0';
        wait for CLK_PERIOD * 2;  -- Wait after reset

        -- Assert start
        start <= '1';
        wait for CLK_PERIOD;
        start <= '0';

        -- Wait for completion
        wait until done = '1' for CLK_PERIOD * 30;
        if done = '1' then
            report "Simulation completed successfully at time: " & time'image(now) severity note;
            report "out_0 = " & to_hex(std_logic_vector(out_0)) severity note;
            report "out_1 = " & to_hex(std_logic_vector(out_1)) severity note;
            report "out_2 = " & to_hex(std_logic_vector(out_2)) severity note;
            report "out_3 = " & to_hex(std_logic_vector(out_3)) severity note;
            report "out_4 = " & to_hex(std_logic_vector(out_4)) severity note;
            report "out_5 = " & to_hex(std_logic_vector(out_5)) severity note;
            report "out_6 = " & to_hex(std_logic_vector(out_6)) severity note;
            report "out_7 = " & to_hex(std_logic_vector(out_7)) severity note;
        else
            report "Simulation timeout: done never asserted" severity error;
            assert false report "Test Case 1 failed" severity failure;
        end if;

        wait for CLK_PERIOD * 5;  -- Brief pause

        -- Test Case 2: Reset during operation
        report "Test Case 2: Reset during operation" severity note;
        start <= '1';
        wait for CLK_PERIOD;
        start <= '0';
        wait for CLK_PERIOD * 10;  -- Let it run for 10 cycles (mid-permutation)

        reset <= '1';  -- Assert reset mid-operation
        wait for CLK_PERIOD * 2;
        reset <= '0';
        wait for CLK_PERIOD * 2;

        -- Restart the process
        start <= '1';
        wait for CLK_PERIOD;
        start <= '0';

        -- Wait for completion again
        wait until done = '1' for CLK_PERIOD * 30;
        if done = '1' then
            report "Simulation completed successfully after reset at time: " & time'image(now) severity note;
            report "out_0 = " & to_hex(std_logic_vector(out_0)) severity note;
            report "out_1 = " & to_hex(std_logic_vector(out_1)) severity note;
            report "out_2 = " & to_hex(std_logic_vector(out_2)) severity note;
            report "out_3 = " & to_hex(std_logic_vector(out_3)) severity note;
            report "out_4 = " & to_hex(std_logic_vector(out_4)) severity note;
            report "out_5 = " & to_hex(std_logic_vector(out_5)) severity note;
            report "out_6 = " & to_hex(std_logic_vector(out_6)) severity note;
            report "out_7 = " & to_hex(std_logic_vector(out_7)) severity note;
        else
            report "Simulation timeout: done never asserted after reset" severity error;
            assert false report "Test Case 2 failed" severity failure;
        end if;

        -- End simulation
        wait for CLK_PERIOD * 5;
        report "Simulation finished" severity note;
        assert false report "Simulation completed - stopping now" severity failure;
        wait;
    end process;

end behavior;
library ieee;
use ieee.std_logic_1164.all;
 
entity clock_enable_generator_test is
end clock_enable_generator_test;
 
architecture behavior of clock_enable_generator_test is 
 
    -- component declaration for the unit under test (uut)
 
    component clock_enable_generator
	 generic ( divider : positive );
    port(
         clk : in  std_logic;
         clk_enable : out  std_logic
        );
    end component;
    

   --inputs
   signal clk : std_logic := '0';

 	--outputs
   signal clk_enable : std_logic;

   -- clock period definitions
   constant clk_period : time := 10 ns;
begin
 
	-- instantiate the unit under test (uut)
   uut: clock_enable_generator
		generic map ( divider => 10 )
		port map (
          clk => clk,
          clk_enable => clk_enable
        );

   -- clock process definitions
   clk_process : process
   begin
		clk <= '0';
		wait for clk_period/2;
		clk <= '1';
		wait for clk_period/2;
   end process;

   -- stimulus process
   stim_proc: process
   begin		
      -- hold reset state for 100 ns.
      wait for 100 ns;	

      wait until clk = '1';
		
		wait for 100 ns;
		
		assert clk = '0';

      wait;
   end process;

end;

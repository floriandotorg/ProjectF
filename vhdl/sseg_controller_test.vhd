library ieee;
use ieee.std_logic_1164.all;
use work.sseg.all;
 
entity sseg_controller_test is
end sseg_controller_test;
 
architecture behavior of sseg_controller_test is 
 
    -- component declaration for the unit under test (uut)
 
    component sseg_controller
		port ( clk : in  std_logic;
				 clk_enable : in  std_logic;
				 values : in sseg_value_arr;
				 sseg : out  std_logic_vector (7 downto 0);
				 anodes : out  std_logic_vector (3 downto 0));
    end component;
    

   --inputs
   signal clk : std_logic := '0';
	
	--outputs
	signal sseg : std_logic_vector (7 downto 0);
	signal anodes : std_logic_vector (3 downto 0);

   -- clock period definitions
   constant clk_period : time := 10 ns;
begin
 
	-- instantiate the unit under test (uut)
   uut: sseg_controller
		port map (
          clk => clk,
          clk_enable => '1',
			 values => ("0001","1010","1101","1111"),
			 sseg => sseg,
			 anodes => anodes
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


      wait;
   end process;

end;

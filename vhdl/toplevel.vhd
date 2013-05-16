library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.sseg.all;

entity toplevel is
	port ( clk : in std_logic;
		    sseg : out  std_logic_vector (7 downto 0);
		    anodes : out  std_logic_vector (3 downto 0);
		    leds : out std_logic_vector (7 downto 0);
		    switches : in std_logic_vector (7 downto 0) );
end toplevel;

architecture behavioral of toplevel is
	constant freq : positive := 50000000; -- 50 MHz
	
	subtype word_type is std_logic_vector (7 downto 0);
	
	signal cpu_clk_enable : std_logic;
	
	signal sseg_clk_enable : std_logic;
	signal sseg_values : sseg_value_arr;
	
	signal addr_bus : word_type;
	signal data_bus : word_type;
	
	signal pc : word_type;
	
	signal memory_data_bus : word_type;
	
	type ucode_state_type is
		( inc_pc,
		  load_instr,
		  fetch_instr, 
		  nop,
		  halt );

	signal ucode_state : ucode_state_type := load_instr;
	
	signal clock_toggel_enable : std_logic;
	signal clock_toggel : std_logic := '1';

begin
	sseg_clock : entity work.clock_enable_generator(behavioral)
		generic map ( divider => freq/500 )
		port map (clk => clk, clk_enable => sseg_clk_enable); 
		
	clock_toggel_clock : entity work.clock_enable_generator(behavioral)
		generic map ( divider => freq/2 )
		port map (clk => clk, clk_enable => clock_toggel_enable); 

	cpu_clock : entity work.clock_enable_generator(behavioral)
		generic map ( divider => freq )
		port map (clk => clk, clk_enable => cpu_clk_enable); 

	sseg_controller : entity work.sseg_controller(behavioral)
		port map ( clk => clk,
  		           clk_enable => sseg_clk_enable,
					  values => sseg_values,
					  sseg => sseg,
					  anodes => anodes ); 
	
	leds(0) <= clock_toggel;
	
	with ucode_state select
		leds(7 downto 1) <= "1000000" when inc_pc,
		                    "0100000" when load_instr,
							     "0010000" when fetch_instr,
							     "0001000" when nop,
							     "0000100" when halt,
							     "0000000" when others;
	
	with switches (0) select
		sseg_values <= ( addr_bus (7 downto 4), addr_bus (3 downto 0), data_bus (7 downto 4), data_bus (3 downto 0) ) when '1',
		               ( pc (7 downto 4), pc (3 downto 0), "1111", "1111" ) when others;
	
	--cpu_clk_enable <= '1';
	
	clk_tgl : process (clk) is
	begin
		if rising_edge(clk) then
			if clock_toggel_enable = '1' then
				clock_toggel <= not clock_toggel;
			end if;
		end if;
	end process;
	
	with ucode_state select
		addr_bus <= pc when load_instr,
                  (others => '0') when others;
	
	with ucode_state select
		data_bus <= memory_data_bus when fetch_instr,
                  (others => '0') when others;
	
	pc_proc : process(clk)
	begin
		if rising_edge(clk) then
			if cpu_clk_enable = '1' then
				if ucode_state = inc_pc then
					pc <= std_logic_vector(unsigned(pc) + 1);
				end if;
			end if;
		end if;
	end process;
	
	memory : process(clk)
		type memory_type is array ( natural range <> ) of word_type;
		variable memory : memory_type (0 to 255) := ("00001111", others => "00000000");
	begin
		if rising_edge(clk) then
			if cpu_clk_enable = '1' then
				case ucode_state is
					when load_instr =>
						memory_data_bus <= memory (to_integer(unsigned(addr_bus)));
					when others =>
						memory_data_bus <= (others => '0');
				end case;
			end if;
		end if;
	end process;
	
	ucode : process(clk)
	begin
		if rising_edge(clk) then
			if cpu_clk_enable = '1' then
				case ucode_state is
					when inc_pc =>
						ucode_state <= load_instr;
					
					when load_instr =>
						ucode_state <= fetch_instr;
						
					when fetch_instr =>
						case data_bus is
							when "00001111" =>
								ucode_state <= nop;
								
							when others =>
								ucode_state <= halt;
						end case;
					
					when nop =>
						ucode_state <= inc_pc;
					
					when halt =>
						ucode_state <= halt;
						
					when others =>
						ucode_state <= inc_pc;
				end case;
			end if;
		end if;
	end process;

end behavioral;


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.sseg.all;


entity toplevel is
	port ( clk : in std_logic;
		    sseg : out  std_logic_vector (7 downto 0);
		    anodes : out  std_logic_vector (3 downto 0);
		    leds : out std_logic_vector (7 downto 0);
		    switches : in std_logic_vector (7 downto 0);
			 mem_oe : out std_logic;
			 mem_we : out std_logic;
			 ram_adv : out std_logic;
			 ram_ce : out std_logic;
			 ram_clk : out std_logic;
			 ram_cre : out std_logic;
			 ram_lb : out std_logic;
			 ram_ub : out std_logic;
			 ram_wait : in std_logic;
			 flash_rp : out std_logic;
			 flash_ce : out std_logic;
			 --flash_st_sts : in std_logic;
			 mem_addr : out std_logic_vector (23 downto 1);
			 mem_data : inout std_logic_vector (15 downto 0) );
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
	signal reg_a : word_type;
	
	signal memory_data_bus : word_type;
	
	type ucode_state_type is
		( inc_pc,
		  load_instr,
		  fetch_instr, 
		  nop,
		  lda_imm_1,
		  lda_imm_2,
		  lda_imm_3,
		  lda_abs_1,
		  lda_abs_2,
		  lda_abs_3,
		  lda_abs_4,
		  sta_abs_1,
		  sta_abs_2,
		  sta_abs_3,
		  halt,
		  panic );

	signal ucode_state : ucode_state_type := load_instr;
	
	signal mem_data_in : std_logic_vector (15 downto 0);
	signal mem_data_out : std_logic_vector (15 downto 0);
	signal mem_data_wr : std_logic;
	
	signal clock_toggel_enable : std_logic;
	signal clock_toggel : std_logic := '1';
	
	constant opcode_lda_imm : word_type := "00000010";
	constant opcode_lda_abs : word_type := "00000100";
	constant opcode_sta_abs : word_type := "00000011";
	constant opcode_nop : word_type := "00001111";
	
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
					  
	mem_data_in <= mem_data;
	mem_data <= mem_data_out when mem_data_wr = '1' else (others => 'Z');
	
	-- RAM enabled
	ram_ce <= '0';
	
	-- control registers disabled
	ram_cre <= '0';
	
	-- only lower byte enabled
	ram_lb <= '0';
	ram_ub <= '1';
	
	-- clock/adv high due to async mode
	ram_clk <= '1';
	ram_adv <= '1';
	
	-- flash disabled
	flash_ce <= '1';
	flash_rp <= '0';
	
	leds(0) <= clock_toggel;
	
	with ucode_state select
		leds(7 downto 1) <= "1000000" when inc_pc,
		                    "0100000" when load_instr,
							     "0010000" when fetch_instr,
							     "0001000" when nop,
							     "0000100" when halt,
								  "1111111" when panic,
							     "0000000" when others;
	
	with switches (0) select
		sseg_values <= ( addr_bus (7 downto 4), addr_bus (3 downto 0), data_bus (7 downto 4), data_bus (3 downto 0) ) when '1',
		               ( pc (7 downto 4), pc (3 downto 0), reg_a (7 downto 4), reg_a (3 downto 0) ) when others;
	
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
		addr_bus <= pc when load_instr | lda_imm_2 | sta_abs_2 | lda_abs_2,
						reg_a when sta_abs_3,
                  (others => '0') when others;
	
	with ucode_state select
		data_bus <= memory_data_bus when fetch_instr | lda_imm_3 | sta_abs_3 | lda_abs_3 | lda_abs_4,
                  (others => '0') when others;
	
	pc_proc : process(clk)
	begin
		if rising_edge(clk) then
			if cpu_clk_enable = '1' then
				if ucode_state = inc_pc or ucode_state = lda_imm_1 or ucode_state = sta_abs_1 or ucode_state = lda_abs_1 then
					pc <= std_logic_vector(unsigned(pc) + 1);
				end if;
			end if;
		end if;
	end process;
	
	reg_a_proc : process(clk)
	begin
		if rising_edge(clk) then
			if cpu_clk_enable = '1' then
				if ucode_state = lda_imm_3 or ucode_state = lda_abs_4 then
					reg_a <= data_bus;
				end if;
			end if;
		end if;
	end process;

--	memory : process(clk)
--	begin
--		if rising_edge(clk) then
--			if cpu_clk_enable = '1' then
--				case mem_state is
--					when mem_pre_read =>
--						mem_addr <= (others => '0');
--						mem_oe <= '1';
--						mem_we <= '1';
--						mem_state <= mem_read;
--						mem_data_out <= (others => '0');
--						mem_data_wr <= '0';
--						
--					when mem_read =>
--						mem_addr <= "11110000000000000000000";
--						mem_oe <= '0';
--						mem_we <= '1';
--						mem_state <= mem_pre_write;
--						mem_data_out <= (others => '0');
--						mem_data_wr <= '0';
--						
--					when mem_pre_write =>
--						mem_addr <= (others => '0');
--						mem_oe <= '1';
--						mem_we <= '1';
--						mem_state <= mem_write;
--						mem_data_out <= (others => '0');
--						mem_data_wr <= '0';
--						
--					when mem_write =>
--						mem_addr <= "11110000000000000000000";
--						mem_oe <= '0';
--						mem_we <= '0';
--						mem_state <= mem_pre_read;
--						mem_data_out <= "1111111110001000";
--						mem_data_wr <= '1';
--					end case;
--			end if;
--		end if;
--	end process;

	memory : process(clk)
		type memory_type is array ( natural range <> ) of word_type;
		constant rom : memory_type (0 to 11) := (opcode_nop, opcode_lda_imm, "10101111", opcode_sta_abs, "11100000", opcode_lda_imm, "00000000", opcode_lda_abs, "11100000", others => opcode_nop);
	begin
		if rising_edge(clk) then
			if cpu_clk_enable = '1' then
				case ucode_state is
					when load_instr | lda_imm_2 | sta_abs_2 | lda_abs_2 =>
						if unsigned(addr_bus) > 12 then
							mem_addr <= "000000000000000" & addr_bus;
							memory_data_bus <= mem_data_in (7 downto 0);
							mem_oe <= '0';
							mem_we <= '1';
							mem_data_wr <= '0';
						else
							mem_addr <= (others => '0');
							memory_data_bus <= rom (to_integer(unsigned(addr_bus)));
							mem_oe <= '1';
							mem_we <= '1';
							mem_data_wr <= '0';
						end if;
					when lda_abs_3 =>
						if unsigned(data_bus) > 12 then
							mem_addr <= "000000000000000" & data_bus;
							memory_data_bus <= mem_data_in (7 downto 0);
							mem_oe <= '0';
							mem_we <= '1';
							mem_data_wr <= '0';
						else
							mem_addr <= (others => '0');
							memory_data_bus <= rom (to_integer(unsigned(data_bus)));
							mem_oe <= '1';
							mem_we <= '1';
							mem_data_wr <= '0';
						end if;
					when sta_abs_3 =>
						memory_data_bus <= (others => '0');
						if unsigned(data_bus) > 12 then
							mem_addr <= "000000000000000" & data_bus;
							mem_data_out <= "00000000" & addr_bus;
							mem_oe <= '0';
							mem_we <= '0';
							mem_data_wr <= '1';
						else
							mem_addr <= (others => '0');
							mem_oe <= '1';
							mem_we <= '1';
							mem_data_wr <= '0';
						end if;
					when others =>
						mem_addr <= (others => '0');
						memory_data_bus <= (others => '0');
						mem_oe <= '1';
						mem_we <= '1';
						mem_data_wr <= '0';
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
							when opcode_nop =>
								ucode_state <= nop;
								
							when opcode_lda_imm =>
								ucode_state <= lda_imm_1;
								
							when opcode_lda_abs =>
								ucode_state <= lda_abs_1;
								
							when opcode_sta_abs =>
								ucode_state <= sta_abs_1;

							when others =>
								ucode_state <= panic;
						end case;
						
					when lda_imm_1 =>
						ucode_state <= lda_imm_2;
					
					when lda_imm_2 =>
						ucode_state <= lda_imm_3;
					
					when lda_imm_3 =>
						ucode_state <= inc_pc;
						
					
					when lda_abs_1 =>
						ucode_state <= lda_abs_2;
					
					when lda_abs_2 =>
						ucode_state <= lda_abs_3;
						
					when lda_abs_3 =>
						ucode_state <= lda_abs_4;
						
					when lda_abs_4 =>
						ucode_state <= inc_pc;
						
					
					when sta_abs_1 =>
						ucode_state <= sta_abs_2;
					
					when sta_abs_2 =>
						ucode_state <= sta_abs_3;
					
					when sta_abs_3 =>
						ucode_state <= inc_pc;
						
					
					when nop =>
						ucode_state <= inc_pc;
						
					
					when halt =>
						ucode_state <= halt;
						
						
					when others =>
						ucode_state <= panic;
				end case;
			end if;
		end if;
	end process;

end behavioral;


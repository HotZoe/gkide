local helper = require('functional.helper')(after_each)
local ok = helper.ok
local eq = helper.eq
local NIL = helper.NIL
local nvim = helper.nvim
local clear = helper.clear
local funcs = helper.funcs
local curtab = helper.curtab
local tabpage = helper.tabpage
local request = helper.request
local command = helper.command
local meth_pcall = helper.meth_pcall
local curtabmeths = helper.curtabmeths

describe('api/tabpage', function()
  before_each(clear)

  describe('list_wins and get_win', function()
    it('works', function()
      nvim('command', 'tabnew')
      nvim('command', 'vsplit')
      local tab1, tab2 = unpack(nvim('list_tabpages'))
      local win1, win2, win3 = unpack(nvim('list_wins'))
      eq({win1},  tabpage('list_wins', tab1))
      eq({win2, win3},  tabpage('list_wins', tab2))
      eq(win2, tabpage('get_win', tab2))
      nvim('set_current_win', win3)
      eq(win3, tabpage('get_win', tab2))
    end)
  end)

  describe('{get,set,del}_var', function()
    it('works', function()
      curtab('set_var', 'lua', {1, 2, {['3'] = 1}})
      eq({1, 2, {['3'] = 1}}, curtab('get_var', 'lua'))
      eq({1, 2, {['3'] = 1}}, nvim('eval', 't:lua'))
      eq(1, funcs.exists('t:lua'))
      curtabmeths.del_var('lua')
      eq(0, funcs.exists('t:lua'))
      eq({false, 'Key does not exist: lua'}, meth_pcall(curtabmeths.del_var, 'lua'))
      curtabmeths.set_var('lua', 1)
      command('lockvar t:lua')
      eq({false, 'Key is locked: lua'}, meth_pcall(curtabmeths.del_var, 'lua'))
      eq({false, 'Key is locked: lua'}, meth_pcall(curtabmeths.set_var, 'lua', 1))
    end)

    it('tabpage_set_var returns the old value', function()
      local val1 = {1, 2, {['3'] = 1}}
      local val2 = {4, 7}
      eq(NIL, request('tabpage_set_var', 0, 'lua', val1))
      eq(val1, request('tabpage_set_var', 0, 'lua', val2))
    end)

    it('tabpage_del_var returns the old value', function()
      local val1 = {1, 2, {['3'] = 1}}
      local val2 = {4, 7}
      eq(NIL,  request('tabpage_set_var', 0, 'lua', val1))
      eq(val1, request('tabpage_set_var', 0, 'lua', val2))
      eq(val2, request('tabpage_del_var', 0, 'lua'))
    end)
  end)

  describe('get_number', function()
    it('works', function()
      local tabs = nvim('list_tabpages')
      eq(1, tabpage('get_number', tabs[1]))

      nvim('command', 'tabnew')
      local tab1, tab2 = unpack(nvim('list_tabpages'))
      eq(1, tabpage('get_number', tab1))
      eq(2, tabpage('get_number', tab2))

      nvim('command', '-tabmove')
      eq(2, tabpage('get_number', tab1))
      eq(1, tabpage('get_number', tab2))
    end)
  end)

  describe('is_valid', function()
    it('works', function()
      nvim('command', 'tabnew')
      local tab = nvim('list_tabpages')[2]
      nvim('set_current_tabpage', tab)
      ok(tabpage('is_valid', tab))
      nvim('command', 'tabclose')
      ok(not tabpage('is_valid', tab))
    end)
  end)
end)

#!/usr/bin/env python
"""
C++ source code generator to create bindings from Neovim API functions using Qt types/signals/slots
"""
import re
import os
import sys
import jinja2
import msgpack
import datetime
import subprocess

INPUT = 'bindings'

def decutf8(inp):
    """
    Recursively decode bytes as utf8 into unicode
    """
    if isinstance(inp, bytes):
        return inp.decode('utf8')
    elif isinstance(inp, list):
        return [decutf8(x) for x in inp]
    elif isinstance(inp, dict):
        return {decutf8(key):decutf8(val) for key,val in inp.items()}
    else:
        return inp

def get_api_info(nvim):
    """
    Call the neovim binary to get the api info
    """
    args = [nvim, '--api-info']
    info = subprocess.check_output(args)
    return decutf8(msgpack.unpackb(info))

def generate_file(name, outpath, **kw):
    from jinja2 import Environment, FileSystemLoader
    env=Environment(loader=FileSystemLoader(INPUT), trim_blocks=True)
    template = env.get_template(name)
    with open(os.path.join(outpath, name), 'w') as fp:
        fp.write(template.render(kw))

class UnsupportedType(Exception):
    """
    Just pass the unsupported type, does nothing
    """
    pass

class NeovimTypeVal:
    """
    Representation for Neovim Parameter/Return
    """
    # msgpack simple types types
    MsgpackSimpleTypeMap = {
        'void': 'void',
        'Array': 'QVariantList',
        'String': 'QByteArray',
        'Object': 'QVariant',
        'Integer': 'int64_t',
        'Boolean': 'bool',
        'Dictionary': 'QVariantMap',
    }

    # msgpack extension types
    MsgpackExtensionTypeMap = {
        'Window': 'int64_t',
        'Buffer': 'int64_t',
        'Tabpage': 'int64_t',
    }

    # nvim pair type
    NvimPairType = 'ArrayOf(Integer, 2)'

    # nvim unbound Array types
    NvimUnboundArrayType = re.compile('ArrayOf\(\s*(\w+)\s*\)')

    def __init__(self, typename, name=''):
        self.name = name
        self.neovim_type = typename
        self.msgpack_ext = False
        self.native_type = NeovimTypeVal.nativeType(typename)

        self.sendmethod = 'send'
        self.decodemethod = 'decodeMsgpack'

        if typename in self.MsgpackSimpleTypeMap:
            pass
        elif typename in self.MsgpackExtensionTypeMap:
            self.msgpack_ext = True
            # FIXME
            #self.sendmethod = 'send%s' % typename
            #self.decodemethod = 'decodeMsgpackAs%s' % typename
        elif self.NvimUnboundArrayType.match(typename):
            m = self.NvimUnboundArrayType.match(typename)
            elemtype = m.groups()[0]
            self.sendmethod = 'sendArrayOf'
        elif typename == self.NvimPairType:
            self.native_type = 'QPoint'
        else:
            raise UnsupportedType(typename)

    @classmethod
    def nativeType(cls, typename):
        """
        Return the native type for this Neovim type.
        """
        if typename == 'void':
            return typename
        elif typename in cls.MsgpackSimpleTypeMap:
            return cls.MsgpackSimpleTypeMap[typename]
        elif typename in cls.MsgpackExtensionTypeMap:
            return cls.MsgpackExtensionTypeMap[typename]
        elif cls.NvimUnboundArrayType.match(typename):
            m = cls.NvimUnboundArrayType.match(typename)
            return 'QList<%s>' % cls.nativeType(m.groups()[0])
        elif typename == cls.NvimPairType:
            return 'QPoint'
        raise UnsupportedType(typename)

class Function:
    """
    Representation for a Neovim API Function
    """
    # Attributes names that we support, see src/function.c for details
    attr_values = ['name', 'parameters', 'return_type', 'can_fail', 'deprecated_since',
                   'since', 'method', 'async', 'impl_name', 'noeval', 'receives_channel_id']
    __KNOWN_ATTRIBUTES = set(attr_values)

    def __init__(self, nvim_fun):
        self.fun = nvim_fun
        self.name =  self.fun['name']
        self.valid = False
        self.parameters = []
        
        try:
            self.return_type = NeovimTypeVal(self.fun['return_type'])
            for param in self.fun['parameters']:
                self.parameters.append(NeovimTypeVal(*param))
        except UnsupportedType as ex:
            print('Found unsupported type(%s) when adding function %s(), skipping' % (ex, self.name))
            return

        u_attrs = self.unknown_attributes()
        if u_attrs:
            print('Found unknown attributes for function %s: %s' % (self.name, u_attrs))

        self.argcount = len(self.parameters)

        # Build the argument string - makes it easier for the templates
        self.argstring = ', '.join(['%s %s' % (tv.native_type, tv.name) for tv in self.parameters])
        self.valid = True

    def is_method(self):
        return self.fun.get('method', False)

    def is_async(self):
        return self.fun.get('async', False)

    def deprecated(self):
        return self.fun.get('deprecated_since', None)

    def unknown_attributes(self):
        attrs = set(self.fun.keys()) - Function.__KNOWN_ATTRIBUTES
        return attrs

    def real_signature(self):
        params = ''
        for p in self.parameters:
            params += '%s %s' % (p.native_type, p.name)
            params += ', '
        notes = ''
        return '%s %s(%s) %s' % (self.return_type.native_type,self.name,params, notes)

    def signature(self):
        params = ''
        for p in self.parameters:
            params += '%s %s' % (p.neovim_type, p.name)
            params += ', '
        notes = ''
        return '%s %s(%s) %s' % (self.return_type.neovim_type,self.name,params, notes)

def print_api(api):
    """
    Show Neovim API Function information in stdout
    """
    for key in api.keys():
        if key == 'functions':
            print('Functions')
            for f in api[key]:
                fundef = Function(f)
                if not fundef.valid:
                    continue
                sig = fundef.signature()
                realsig = fundef.real_signature()
                print('\t%s'% sig)
                deprecated = fundef.deprecated()
                if deprecated:
                    print('\t- Deprecated: %d' % deprecated)
                if sig != realsig:
                    print('\t- Native: %s\n' % realsig)
            print('')
        elif key == 'types':
            print('Data Types')
            for typ in api[key]:
                print('\t%s' % typ)
            print('')
        elif key == 'error_types':
            print('Error Types')
            for err,desc in api[key].items():
                print('\t%s:%d' % (err,desc['id']))
            print('')
        elif key == 'features':
            pass
        else:
            print('Unknown API info attribute: %s' % key)

if __name__ == '__main__':

    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print('Usage:')
        print('\tnvimbinding <nvim>')
        print('\tnvimbinding <nvim> [output-path]')
        sys.exit(-1)

    nvim = sys.argv[1]
    outpath = None if len(sys.argv) < 3 else sys.argv[2]

    try:
        api = get_api_info(sys.argv[1])
    except subprocess.CalledProcessError as ex:
        print(ex)
        sys.exit(-1)

    if outpath:
        print('Writing auto generated bindings to %s' % outpath)
        if not os.path.exists(outpath):
            os.makedirs(outpath)
        for name in os.listdir(INPUT):
            if name.startswith('.'):
                continue
            if not name.endswith('.h') and not name.endswith('.hpp') and not name.endswith('.cpp'):
                continue

            env = {}
            env['date'] = datetime.datetime.now()
            functions = [Function(f) for f in api['functions'] if f['name'] != 'vim_get_api_info']
            env['functions'] = [f for f in functions if f.valid]
            exttypes = { typename:info['id'] for typename,info in api['types'].items()}
            env['exttypes'] = exttypes
            generate_file(name, outpath, **env)

    else:
        print('API info for %s:' % nvim)
        print_api(api)

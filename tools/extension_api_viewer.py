from __future__ import annotations
from typing import List,Dict,Any
import json
from os.path import exists

filecontent = None
if not exists("./extension_api.json"):
    raise RuntimeError("Please place the extension_api.json in current working directory.")
with open("./extension_api.json","r") as fw:
    filecontent = fw.read()
filecontent = json.loads(filecontent)

sizes = {}
classes: Dict[str,"ClassDocs"] = {}
categories: Dict[str,List[str]] = {
    "Globals":["GlobalScope"],
    "Variant":["Object"],
    "Nodes":[],
    "3D":[],
    "2D":[],
    "Control":[],
    "Resources":[],
    "OtherObjects":[],
    "Servers":[],
    "Native":[]
}

show_all_flag = False
show_inheritance_too = False

for i in filecontent["builtin_class_sizes"][3]["sizes"]:
    sizes[i["name"]] = i["size"]

class ClassDocs:
    def __init__(self,data = None):
        self.symbol_type: Dict[str,str] = {}
        if data == None:
            self.name: str = ""
            self.brief_description: str | None = None
            self.description: str | None = None
            self.size: int | None = None
            self.members: Dict[str,str] = {}
            self.constants: Dict[str, str] = {}
            self.operators: Dict[str,str] = {}
            self.methods: Dict[str,str] = {}
            self.constructors: Dict[str,str] = {}
            self.signals: Dict[str,str] = {}
            self.enums: Dict[str,Dict[str,str]] = {}
            self.inheriters: List[str] = []
            self.inherits_from: List[str] = []
        else:
            self.name: str = data["name"]
            self.brief_description: str | None = data.get("brief_description")
            self.description: str | None = data.get("description")
            if self.description is not None:
                self.description = self.description.replace("\\n","\n")
            self.size: int | None = sizes.get(self.name,None)
            self.members: Dict[str,str] = {}
            self.constants: Dict[str, str] = {}
            self.operators: Dict[str,str] = {}
            self.methods: Dict[str,str] = {}
            self.constructors: Dict[str,str] = {}
            self.signals: Dict[str,str] = {}
            self.enums: Dict[str,Dict[str,str]] = {}
            self.inheriters: List[str] = []
            self.inherits_from: List[str] = []
            for v in data.get("members",[]):
                self.members[v["name"]] = f"{v['type']} {v['name']}\n"+v.get("description","No description provided.")
            for v in data.get("properties",[]):
                self.members[v["name"]] = f"{v['type']} {v['name']}\t( {'get: {}{}'.format(v.get('getter'),', ' if 'setter' in v else '') if 'getter' in v else ''}{'set: {}'.format(v.get('setter'),) if 'setter' in v else ''} )\n"+v.get("description","No description provided.")
            for v in data.get("constants",[]):
                self.constants[v["name"]] = f"{v.get('type','int')} {v['name']}"+(f" = {v['value']}" if "value" in v else "")+"\n"+v.get("description","No description provided.")
            for v in data.get("operators",[]):
                self.operators["operator_"+v["name"]] = f"{v['return_type']} operator {v['name']}({v.get('right_type','')})\n"+v.get("description","No description provided.")
            for v in data.get("methods",[]):
                args = v.get("arguments",[])
                self.methods[v["name"]] = "{} {}({}{}){}{}\n{}".format(v.get('return_type','void'),v["name"],str.join(', ',(a['type']+' '+a['name'] for a in args)),(f"{', ' if len(args) > 0 else ''}..." if v["is_vararg"] else ""),(' const' if v.get("is_const") else ''),(' static' if v.get("is_static") else ''),v.get("description", "No description provided."))
            for v in data.get("constructors",[]):
                args = v.get("arguments",[])
                self.constructors[f"{self.name}({str.join(', ',(a['type']+' '+a['name'] for a in args))})"] = f"{self.name}({str.join(', ',(a['type']+' '+a['name'] for a in args))})\n"+v.get("description","No description provided.")
            for v in data.get("signals",[]):
                args = v.get("arguments",[])
                self.signals[v["name"]] = "signal {}\n{}".format(v["name"],(f"({str.join(', ',(a['type']+' '+a['name'] for a in args))})" if len(args) > 0 else ""),v.get("description", "No description provided."))
            for vv in data.get("enums",[]):
                e = {}
                for v in vv.get("values",[]):
                    e[v["name"]] = f"{v['name']} = {str(v['value'])}\n"+v.get("description", "No description provided.")
                self.enums[vv["name"]] = e
            inherits = data.get("inherits")
            if inherits is not None:
                self.inherits_from.append(inherits)
            self._update()
    def _recursive_get_inherited_stuff(self) -> Dict[Dict[str,Any]]:
        inherited_stuff = {}
        if len(self.inherits_from)>0:
            inherited_stuff = classes[self.inherits_from[0]]._recursive_get_inherited_stuff()
        stuff = {
            "members": {},
            "constants": {},
            "operators": {},
            "methods": {},
            "constructors": {},
            "signals": {},
            "enums": {}
        }
        stuff["members"].update(self.members)
        stuff["constants"].update(self.constants)
        stuff["operators"].update(self.operators)
        stuff["methods"].update(self.methods)
        stuff["constructors"].update(self.constructors)
        stuff["signals"].update(self.signals)
        stuff["enums"].update(self.enums)
        inherited_stuff[self.name]=stuff
        return inherited_stuff    
    def _update(self):
        self.symbol_type.clear()
        for k in self.members.keys():
            self.symbol_type[k]="member"
        for k in self.constants.keys():
            self.symbol_type[k]="constant"
        for k in self.operators.keys():
            self.symbol_type[k]="operator"
        for k in self.methods.keys():
            self.symbol_type[k]="method"
        for k in self.constructors.keys():
            self.symbol_type[k]="constructor"
        for k in self.signals.keys():
            self.symbol_type[k]="signal"
        for k in self.enums.keys():
            self.symbol_type[k]="enum"
    def __repr__(self) -> str:
        string = f"class {self.name} {f'(size={self.size})' if self.size is not None else ''}\n"
        string += (self.brief_description if self.brief_description is not None else "No description provided.")+"\n"
        if len(self.inherits_from) > 0:
            string += f"\tInherits: {str.join(' < ',self.inherits_from)}\n"
        if len(self.inheriters) > 0:
            string += f"\tInherited By: {str.join(', ',self.inheriters)}\n"
        if not show_inheritance_too:
            if len(self.members) > 0:
                string += "Members\n"
                keys = list(self.members.keys())
                keys.sort()
                for k in keys:
                    string += "\t"+self.members[k].split("\n")[0]+"\n"
            if len(self.methods) > 0:
                string += "Methods\n"
                keys = list(self.methods.keys())
                keys.sort()
                for k in keys:
                    string += "\t"+self.methods[k].split("\n")[0]+"\n"
            if len(self.constructors) > 0:
                keys = list(self.constructors.keys())
                keys.sort()
                for k in keys:
                    string += "\t"+self.constructors[k].split("\n")[0]+"\n"
            if len(self.operators) > 0:
                keys = list(self.operators.keys())
                keys.sort()
                for k in keys:
                    string += "\t"+self.operators[k].split("\n")[0]+"\n"
            if len(self.constants) > 0:
                string += "Constants\n"
                keys = list(self.constants.keys())
                keys.sort()
                for k in keys:
                    string += "\t"+str.join("\n\t\t",self.constants[k].split("\n"))+"\n"
            if len(self.enums) > 0:
                string += "Enums\n"
                keys = list(self.enums.keys())
                keys.sort()
                for k in keys:
                    enum = self.enums[k]
                    string += f"enum {k}:\n"
                    for item in enum.values():
                        enumitem = item.split("\n")
                        string += f"\t{k} {enumitem[0]}\n"
                        string+= "\t\t{}\n".format(str.join('\n',enumitem[1:]))
            if len(self.signals) > 0:
                string += "Signals\n"
                keys = list(self.signals.keys())
                keys.sort()
                for k in keys:
                    s = self.signals[k].split("\n")
                    string += "\t"+str.join("",s)+"\n"
            if not show_all_flag: return string[:-1]
            if len(self.members) > 0:
                string += "Member Descriptions\n"
                keys = list(self.members.keys())
                keys.sort()
                for k in keys:
                    propitem = self.members[k].split("\n")
                    string += propitem[0]+"\n\t"+str.join("\n\t",propitem[1:])+"\n"
            if len(self.methods) > 0:
                string += "Method Descriptions\n"
                keys = list(self.methods.keys())
                keys.sort()
                for k in keys:
                    methoditem = self.methods[k].split("\n")
                    string += methoditem[0]+"\n\t"+str.join("\n\t",methoditem[1:])+"\n"
        else:
            inh = [self.name]+self.inherits_from
            stuff = self._recursive_get_inherited_stuff()
            for cls in inh:
                if len(stuff[cls]["members"]) > 0:
                    string += f"Properties {f'(inherited from {cls})' if cls != self.name else ''}\n"
                    for p in stuff[cls]["members"].values():
                        string += "\t"+p.split("\n")[0]+"\n"
            for cls in inh:
                if len(stuff[cls]["methods"]) > 0:
                    string += f"Methods {f'(inherited from {cls})' if cls != self.name else ''}\n"
                    for p in stuff[cls]["methods"].values():
                        string += "\t"+p.split("\n")[0]+"\n"
                if len(stuff[cls]["constructors"]) > 0:
                    for p in stuff[cls]["constructors"].values():
                        string += "\t"+p.split("\n")[0]+"\n"
                if len(stuff[cls]["operators"]) > 0:
                    for p in stuff[cls]["operators"].values():
                        string += "\t"+p.split("\n")[0]+"\n"
            for cls in inh:
                if len(stuff[cls]["constants"]) > 0:
                    string += f"Constants {f'(inherited from {cls})' if cls != self.name else ''}\n"
                    for p in stuff[cls]["constants"].values():
                        string += "\t"+str.join("\n\t",p.split("\n"))+"\n"
            for cls in inh:
                if len(stuff[cls]["enums"]) > 0:
                    string += f"Enums {f'(inherited from {cls})' if cls != self.name else ''}\n"
                    for k,enum in stuff[cls]["enums"].items():
                        string += f"enum {enum}:\n"
                        for item in enum.values():
                            enumitem = item.split("\n")
                            string += f"\t{k} {enumitem[0]}\n"
                            if show_all_flag:
                                string+= "\t\t{}\n".format(str.join("\n",enumitem[1:]))
            for cls in inh:
                if len(stuff[cls]["members"]) > 0:
                    string += f"Property Descriptions {f'(inherited from {cls})' if cls != self.name else ''}\n"
                    for p in stuff[cls]["members"].values():
                        methoditem = p.split("\n")
                        string += methoditem[0]+"\n\t"+str.join("\n\t",methoditem)+"\n"
            for cls in inh:
                if len(stuff[cls]["methods"]) > 0:
                    string += f"Method Descriptions {f'(inherited from {cls})' if cls != self.name else ''}\n"
                    for p in stuff[cls]["methods"].values():
                        methoditem = p.split("\n")
                        string += methoditem[0]+"\n\t"+str.join("\n\t",methoditem)+"\n"
            if not show_all_flag: return string[:-1]
        return string[:-1]
    def _update_inherits_from(self):
        if len(self.inherits_from) != 0:
            if self.inherits_from[-1] == "Object": 
                c = classes["Object"]
                if self.name not in c.inheriters: c.inheriters.append(self.name)
                return # already done
            elif len(self.inherits_from) == 1:
                c = classes[self.inherits_from[0]]
                c.inheriters.append(self.name)
                c._update_inherits_from()
                self.inherits_from += c.inherits_from

class StructRepr:
    def __init__(self, data):
        self.name = data["name"]
        self.data = data["format"].replace(";",";\n")
        self.members = {(v.split(" ")[1]): v for v in data["format"].split(';')}
        self.symbol_type = {k:"member" for k in self.members.keys()}
    def __repr__(self):
        string = f"struct {self.name} "+"{\n"
        for i in self.data.split(";\n"):
            string += f"\t{i};\n"
        return string + "};"

global_scope = ClassDocs()
global_scope.name = "GlobalScope"
global_scope.description = "The global scope"
for v in filecontent["singletons"]:
    global_scope.members[v["name"]] = f"{v['type']} {v['name']}"
for v in filecontent["utility_functions"]:
    args = v.get("arguments",[])
    global_scope.methods[v["name"]] = "{} {}({}{}){}{}\n{}".format(v.get('return_type','void'),v["name"],str.join(', ',(a['type']+' '+a['name'] for a in args)),(f"{', ' if len(args) > 0 else ''}..." if v["is_vararg"] else ""),(' const' if v.get("is_const") else ''),(' static' if v.get("is_static") else ''),v.get("description", "No description provided."))
for vv in filecontent["global_enums"]:
    e = {}
    for v in vv.get("values",[]):
        e[v["name"]] = f"{v['name']} = {str(v['value'])}\n"+v.get("description", "No description provided.")
    global_scope.enums[vv["name"]] = e
classes["GlobalScope"]=global_scope
global_scope._update()

for cls_data in filecontent["builtin_classes"]:
    cls = ClassDocs(cls_data)
    classes[cls.name] = cls
    categories["Variant"].append(cls.name)
for cls_data in filecontent["classes"]:
    cls = ClassDocs(cls_data)
    classes[cls.name] = cls
for cls_data in filecontent["native_structures"]:
    cls = StructRepr(cls_data)
    classes[cls.name] = cls
    categories["Native"].append(cls.name)

for cls in classes.values():
    if hasattr(cls,"_update_inherits_from"): cls._update_inherits_from()

for cls in classes.values():
    if hasattr(cls,"_update_inherits_from"):
        if "Node" == cls.name or "Node" in cls.inherits_from:
            categories["Nodes"].append(cls.name)
        elif "Resource" == cls.name or "Resource" in cls.inherits_from:
            categories["Resources"].append(cls.name)
        elif "Server" in cls.name or any(map(lambda x: "Server" in x,cls.inherits_from)):
            if cls.name in ["TCPServer","UDPServer","DTLSServer"]: continue
            categories["Servers"].append(cls.name)
        elif len(cls.inherits_from)==0: continue
        else:
            categories["OtherObjects"].append(cls.name)
for cls in classes.values():
    if hasattr(cls,"_update_inherits_from"):
        if "Node2D" == cls.name or "Node2D" in cls.inherits_from:
            categories["2D"].append(cls.name)
        elif "Node3D" == cls.name or "Node3D" in cls.inherits_from:
            categories["3D"].append(cls.name)
        elif "Control" == cls.name or "Control" in cls.inherits_from:
            categories["Control"].append(cls.name)

class Shell:
    def __init__(self):
        pass
    
    def _on_help(self):
        print(
            "LIST:\t\tlist category   Lists all categories",
            "\t\tlist <category> Lists classes in category",
            "INSPECT:\tinspect <class> Inspects class",
            "\t\tinspect <class> <name> Inspects class's property/enum/etc.",
            "FLAGS:\t\tflag show_all <false/true> Shows methods/property descriptions upon every inspect command",
            "\t\tflag show_inherited <false/true> Shows what was inherited from other classes along side what the class has implemented",
            "EXIT:\t\texit Exits the program.",
        sep="\n")

    def run(self):
        try:
            print("GDExtension Documentation viewer scripted by RadiantUwU")
            print(f"GDExtension API for {filecontent['header']['version_full_name']}")
            print("Type \"help\" for more info on how to use this tool.")
            while True:
                cmd = input("> ")
                lower_cmd = cmd.lower()
                lower_cmd_args = lower_cmd.split(" ")[1:]
                cmd_args = cmd.split(" ")[1:]
                if lower_cmd == "help": self._on_help()
                elif lower_cmd == "\n": pass
                elif lower_cmd.startswith("list"):
                    if len(lower_cmd_args) == 0:
                        print(f"error: no arguments")
                    elif lower_cmd_args[0] == "category":
                        print("\n".join(categories.keys()))
                    elif cmd_args[0] in categories:
                        print("\n".join(categories[cmd_args[0]]))
                    else:
                        print(f"error: category {cmd_args[0]} not found.")
                elif lower_cmd.startswith("inspect"):
                    if len(cmd_args) == 0:
                        print(f"error: i can't inspect you silly")
                    elif len(cmd_args) == 1:
                        if cmd_args[0] in classes:
                            print(classes[cmd_args[0]])
                        else:
                            print(f"error: class {cmd_args[0]} not found.")
                    elif len(cmd_args) == 2:
                        if cmd_args[0] in classes and cmd_args[1] in classes[cmd_args[0]].symbol_type:
                            cls = classes[cmd_args[0]]
                            match cls.symbol_type[cmd_args[1]]:
                                case "member":
                                    print(cls.members[cmd_args[1]])
                                case "method":
                                    print(cls.methods[cmd_args[1]])
                                case "operator":
                                    print(cls.operators[cmd_args[1]])
                                case "constructor":
                                    print(cls.constructors[cmd_args[1]])
                                case "signal":
                                    print(cls.signals[cmd_args[1]])
                                case "enum":
                                    string = f"enum {cmd_args[1]}:\n"
                                    for item in cls.enums[cmd_args[1]].values():
                                        enumitem = item.split("\n")
                                        string += f"\t{k} {enumitem[0]}\n"
                                        string+= "\t\t{}\n".format(str.join('\n',enumitem[1:]))
                                    print(string)
                        elif cmd_args[0] in classes:
                            print(f"error: symbol {cmd_args[1]} in {cmd_args[0]} not found")
                        else:
                            print(f"error: class {cmd_args[0]} not found.")
                    else:
                        print("error: expected 1-2 arguments")
                elif lower_cmd.startswith("flags"):
                    if len(cmd_args) != 2:
                        print(f"error: expected 2 arguments")
                    elif lower_cmd_args[0] == "show_all":
                        if lower_cmd_args[1] == "true":
                            show_all_flag = True
                        elif lower_cmd_args[1] == "false":
                            show_all_flag = False
                        else:
                            print(f"error: expected true or false, got {lower_cmd_args[1]}")
                    elif lower_cmd_args[0] == "show_inherited":
                        if lower_cmd_args[1] == "true":
                            show_inheritance_too = True
                        elif lower_cmd_args[1] == "false":
                            show_inheritance_too = False
                        else:
                            print(f"error: expected true or false, got {lower_cmd_args[1]}")
                    else:
                        print(f"error: unknown flag {lower_cmd_args[0]}")
                elif lower_cmd.startswith("exit"):
                    exit()
                else:
                    print(f"error: couldn't find command {lower_cmd.split(' ')[0]}")
        except KeyboardInterrupt: pass
        except EOFError: pass

if __name__=="__main__":
    Shell().run()

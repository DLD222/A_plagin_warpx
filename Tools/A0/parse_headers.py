# SPDX-License-Identifier: BSD-3-Clause
import os,re,json

ROOT='Source'
def strip_comments(text):
    text=re.sub(r'/\*.*?\*/','',text,flags=re.S)
    text=re.sub(r'//[^\n]*','',text)
    return text

def find_matching(text, start):
    depth=0; i=start
    while i<len(text):
        c=text[i]
        if c=='{': depth+=1
        elif c=='}':
            depth-=1
            if depth==0: return i+1
        i+=1
    return len(text)

def parse_body(body, default_section):
    pub_methods=[]; pub_members=[]; priv=0; prot=0
    section=default_section
    i=0; n=len(body)
    while i<n:
        # skip whitespace
        while i<n and body[i].isspace(): i+=1
        if i>=n: break
        # section label
        m=re.match(r'(public|protected|private)\s*:', body[i:])
        if m:
            section=m.group(1); i+=m.end(); continue
        # collect declaration until ; or { at top level (ignore <> and () nesting)
        start=i; par=0; ang=0; brack=0
        while i<n:
            c=body[i]
            if c=='"':
                j=i+1
                while j<n and body[j]!='"': j+=1
                i=j+1; continue
            if c=='(': par+=1
            elif c==')': par-=1
            elif c=='<': ang+=1
            elif c=='>': ang-=1
            elif c=='[': brack+=1
            elif c==']': brack-=1
            elif c==';' and par==0 and ang==0 and brack==0:
                break
            elif c=='{' and par==0 and ang==0 and brack==0:
                break
            i+=1
        decl=body[start:i].strip()
        term=body[i] if i<n else ''
        if term=='{':
            i=find_matching(body,i)
        elif term==';':
            i+=1
        if not decl: continue
        decl=re.sub(r'\s+',' ',decl)
        decl=re.sub(r'^template\s*<.*?>\s*','',decl,count=1)
        if re.match(r'^(using|typedef|friend|static_assert)\b',decl): continue
        if '(' in decl:
            if section=='public': pub_methods.append(decl)
        else:
            if section=='public': pub_members.append(decl)
            elif section=='private': priv+=1
            elif section=='protected': prot+=1
    return pub_methods,pub_members,priv,prot

classes=[]
for dp,_,fs in os.walk(ROOT):
    for f in sorted(fs):
        if not f.endswith('.H'): continue
        p=os.path.join(dp,f)
        raw=open(p,errors='ignore').read()
        text=strip_comments(raw)
        text='\n'.join(l for l in text.split('\n') if not l.lstrip().startswith('#'))
        for m in re.finditer(r'(?m)^[ \t]*(?:template\s*<[^>]*>\s*)?(class|struct)\s+(?:[A-Z_][A-Z0-9_]*\s+)?([A-Za-z_]\w*)\s*(?:final\s*)?(?::([^{;]+))?\{', text):
            kind=m.group(1); name=m.group(2); bases=m.group(3) or ''
            brace=text.find('{',m.start()); end=find_matching(text,brace)
            body=text[brace+1:end-1]
            pm,pubm,priv,prot=parse_body(body, 'public' if kind=='struct' else 'private')
            classes.append({'file':p,'kind':kind,'name':name,'bases':re.sub(r'\s+',' ',bases).strip(),
                'public_methods':pm,'public_members':pubm,
                'n_public_methods':len(pm),'n_public_members':len(pubm),
                'n_private_members':priv,
                'is_template':bool(re.search(r'template\s*<', text[max(0,m.start()-120):m.start()])),
                'has_virtual':('virtual' in body),'has_pure':('= 0' in body or '=0' in body)})
print('classes/structs:',len(classes),'public methods:',sum(c['n_public_methods'] for c in classes))
json.dump(classes,open('/tmp/header_catalog.json','w'),indent=1)

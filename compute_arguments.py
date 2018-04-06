frame = {'left':0, 'right':707, 'top':0, 'bottom':744}

rectangles = [
            {'left':0 + 329,'right':141 + 329,'top':0 + 250,'bottom':40 + 250 },
            {'left':0 + 523,'right':162 + 523,'top':0 + 235,'bottom':56 + 235 },
            {'left':0 + 114,'right':64 + 114,'top':0 + 42,'bottom':104 + 42 },
            {'left':0 + 329,'right':120 + 329,'top':0 + 330,'bottom':120 + 330 },
            {'left':0 + 489,'right':141 + 489,'top':0 + 394,'bottom':56 + 394 },
            {'left':0 + 22,'right':267 + 22,'top':0 + 186,'bottom':72 + 186 },
            {'left':0 + 218,'right':71 + 218,'top':0 + 10,'bottom':136 + 10 },
            {'left':0 + 211,'right':78 + 211,'top':0 + 298,'bottom':72 + 298 },
            {'left':0 + 183,'right':211 + 183,'top':0 + 650,'bottom':72 + 650 },
            {'left':0 + 10,'right':133 + 10,'top':0 + 634,'bottom':88 + 634 },
            {'left':0 + 183,'right':106 + 183,'top':0 + 506,'bottom':104 + 506 },
            {'left':0 + 565,'right':105 + 565,'top':0 + 490,'bottom':120 + 490 },
            {'left':0 + 523,'right':57 + 523,'top':0 + 139,'bottom':56 + 139 },
            {'left':0 + 564,'right':63 + 564,'top':0 + 650,'bottom':72 + 650 },
            {'left':0 + 329,'right':154 + 329,'top':0 + 122,'bottom':88 + 122 },
            {'left':0 + 10,'right':98 + 10,'top':0 + 538,'bottom':56 + 538 },
            {'left':0 + 177,'right':112 + 177,'top':0 + 410,'bottom':56 + 410 },
            {'left':0 + 59,'right':112 + 59,'top':0 + 298,'bottom':72 + 298 },
            {'left':0 + 87,'right':50 + 87,'top':0 + 410,'bottom':56 + 410 },
            {'left':0 + 329,'right':196 + 329,'top':0 + 490,'bottom':120 + 490 }
        ]

links = [
            {'source':1, 'target':14 },
            {'source':2, 'target':14 },
            {'source':2, 'target':5 },
            {'source':3, 'target':4 },
            {'source':5, 'target':17 },
            {'source':6, 'target':14 },
            {'source':6, 'target':5 },
            {'source':7, 'target':0 },
            {'source':7, 'target':16 },
            {'source':8, 'target':9 },
            {'source':9, 'target':15 },
            {'source':10, 'target':18 },
            {'source':10, 'target':16 },
            {'source':10, 'target':9 },
            {'source':11, 'target':13 },
            {'source':12, 'target':14 },
            {'source':14, 'target':7 },
            {'source':16, 'target':3 },
            {'source':17, 'target':7 },
            {'source':19, 'target':10 },
            {'source':19, 'target':7 },
            {'source':19, 'target':8 },
            {'source':19, 'target':11 }
        ]

print("{:04x}{:04x}{:04x}{:04x}".format(frame['left'], frame['right'], frame['top'], frame['bottom']))
print("".join("{:04x}{:04x}".format(r['right']-r['left'], r['bottom']-r['top']) for r in rectangles))
print("".join("{:04x}{:04x}".format(r['left'], r['top']) for r in rectangles))
print("".join("{:02x}{:02x}".format(lk['source'], lk['target']) for lk in links))


pico-8 cartridge // http://www.pico-8.com
version 43
__lua__
-- menu!
autostart=true
levelid=1

function menu_update()
	if (btn(❎) or autostart) begin_game(levelid)
end

function menu_draw()
	cls(0)
	?"press ❎ to start"
end

-->8
-- game!

types={
	{sp=0}, -- 1: root bomb
	{sp=10} -- 2: blade
}

-- state
pl={x=64,y=64}
levels={
	{0,0,16,16}
}
mdata=nil
--1=planning,2=sim
phase=1
bmb={rad=8,id=1}

function set_level(id)
	mdata=levels[id]
	mdata.minx=10
	mdata.miny=10
	mdata.maxx=mdata[3]*8-10
	mdata.maxy=mdata[4]*8-10
	bmb.x=(mdata.minx+mdata.maxx)/2
	bmb.y=(mdata.miny+mdata.maxy)/2
	bmb.cld={}
	phase=1
end

-- cb
function begin_game(id)
	gstate=2
	set_level(id)
	
	if true then
		local att=add_attach(bmb,0.63)
		--add_attach(att,0.65)
		--att=add_attach(att,0.85)
		--add_attach(att,0.9)
		att=add_attach(bmb,0.2)
		--add_attach(att,0.4)
		add_attach(bmb,0)
	end
end

function bmb_to_list()
	local lst={bmb}
	local idx=1
	
	while (idx<=#lst) do
		local cur=lst[idx]
		
		for v in all(cur.cld) do
			add(lst,v)
			local offset=cur.rad+v.rad
			v.x=cur.x+offset*cos(v.rot)
			v.y=cur.y+offset*sin(v.rot)
			v.ax=cur.x+cur.rad*cos(v.rot)
			v.ay=cur.y+cur.rad*sin(v.rot)
		end
		
		idx+=1
	end
	
	return lst
end

function fnd_tgt()
	local lst=bmb_to_list()
	for cand in all(lst) do
		local range=dist(cand.x,cand.y,pl.x,pl.y)
		if range<cand.rad then
			return cand
		end
	end
	
	return nil
end

function add_attach(parent,rot)
	local att={
		id=2,
		rad=4,
		rot=rot,
		cld={}
	}
	add(parent.cld,att)
	return att
end

target=nil
selected=nil
function pln_update()
	target = fnd_tgt()
	if target!=nil then
		dbg("target:found")
	end
	
	if selected==nil then
		if target!=nil then
			-- add!
			if btnp(❎) then
				local att=add_attach(target,0)
				selected=att
			elseif btnp(🅾️) and target!=bmb then
				selected=target
			end
		elseif btnp(❎) or btnp(🅾️) then
			start_sim()
		end
	else
		if (btn(⬅️) or btn(⬆️)) selected.rot+=0.025
		if (btn(➡️) or btn(⬇️)) selected.rot-=0.025
		if (btnp(❎) or btnp(🅾️)) selected=nil
	end
end

actors={}

function start_sim()
	phase=2
	actors={}
	splode(bmb.x,bmb.y,20)
	init_sim()
end

function sim_update()
	if btnp(❎) or btnp(🅾️) then
		phase=1
	else
		tick_sim()
	end
end

function game_update()
	if selected==nil then
		if (btn(⬅️)) pl.x -= 2
		if (btn(➡️)) pl.x += 2
		if (btn(⬆️)) pl.y -= 2
		if (btn(⬇️)) pl.y += 2
		pl.x=clamp(pl.x,mdata.minx,mdata.maxx)
		pl.y=clamp(pl.y,mdata.miny,mdata.maxy)
	end
	
	if (phase==1) then 
		pln_update()
	else
		sim_update()
	end
	
	tick_splode()
end

function draw_pln_bomb()
	local lst=bmb_to_list()
	for ent in all(lst) do
		doblink = (ent==selected) or ((ent==target) and (selected==nil))
	
		if doblink then
			local blnk=flr((t()%1)*4)%2==0
			if blnk then
				if ent==selected then
					pal({2,2,8,8,2,15,15,8,8,15,15,8,8,15,8,2})
				else
					pal({1,1,5,5,5,6,7,13,6,7,7,6,13,6,7,1})
				end
			end
		end
		local sp=types[ent.id].sp
		if sp==0 then
			spr(12,ent.x-8,ent.y-8,2,2)
		else
			spr(sp,ent.x-ent.rad,ent.y-ent.rad)
			spr(3,ent.ax-1,ent.ay-1)
		end
		pal()
	end
end

function game_draw()
	cls(0)
	camera(pl.x-64,pl.y-64)
	map(mdata[1],mdata[2],0,0,mdata[3],mdata[4])
	
	if phase==1 then
		draw_pln_bomb()
	else
		draw_sim()
	end
	
	draw_splode()
	
	if (phase==1 and selected==nil) spr(0,pl.x-3,pl.y-3)
	
	camera()
	rectfill(0,121,128,128,8)
	local action🅾️,action❎
	if selected!=nil then
		action🅾️="stop"
		action❎="stop"
	elseif target!=nil then
		action🅾️="rotate"
		action❎="modify/extend"
	else
		action🅾️="run"
		action❎="run"
	end
	print("🅾️ "..action🅾️.." ❎ "..action❎,1,122,2)
	
	draw_debug()
end

-- explosions

particles={}
excolors={5,9,10,7}

function splode(x,y,num)
	for i=0,num do
		add(particles, {
			x=flr(rnd(16)-8)+x,
			y=flr(rnd(16)-8)+y,
			vx=rnd(3)-1.5,
			vy=rnd(3)-1.5,
			r=flr(rnd(3))+2,
			frames=30+flr(rnd(15))
		})
	end
end

function tick_splode()
	keep={}
	for part in all(particles) do
		part.mod=(part.frames/30+0.3)
		part.x+=part.vx*part.mod
		part.y+=part.vy*part.mod
		part.frames-=1
		if (part.frames>=0) add(keep,part)		
	end
	particles=keep
end

function draw_splode()
	for part in all(particles) do
		local cidx=excolors[min(flr(part.frames/10),#excolors)]
		local rad=part.mod*part.r
		circfill(part.x,part.y,rad,cidx)
	end
end
-->8
-- sim shit

colliders={}
sim_actors={}

function register_aabb(x,y,w,h)
	local aabb={
		x=x,y=y,w=w,h=h,mx=x+w,my=y+h,
		hw=w/2.0,hh=h/2.0,
		draw=function (self)
			rect(self.x,self.y,self.mx,self.my,11)
		end,
		check=function (self,crcl)
			return chk_coll_aabb(self,crcl)
		end
	}
	add(colliders,aabb)
end

function debug_phys()
	crcl={x=pl.x,y=pl.y,rad=3}
	circ(crcl.x,crcl.y,crcl.rad,8)
	
	for c in all(colliders) do
		c:draw()
		local col=c:check(crcl)
		if col!= nil then
			circ(col.hitx,col.hity,1,15)
			dbg(""..col.dir.x.." "..col.dir.y)
		end
	end
end

function make_phys(x,y,vx,vy,vel)
	return {
		x=x,y=y,vx=vx,vy=vy,vel=vel,
		advance=function (self)
			self.x+=self.vx*self.vel
			self.y+=self.vy*self.vel
		end
	}
end

function make_group(x,y,vx,vy,vel)
	return {
		phys=make_phys(x,y,vx,vy,vel),
		actors={},
		links={},
		add_actor=function (self,actor, parent)
			add(self.actors, actor)
			add(self.links, {actor,parent})
		end,
		draw=function (self)
			for cld in all(self.actors) do
				cld:draw(self)
			end
		end
	}
end

function make_actor(id,x,y,vx,vy,vel,rad)
	return {
		phys=make_phys(x,y,vx,vy,vel),
		id=id,
		rad=rad,
		draw=function (self,parent)
			local dx=self.phys.x
			local dy=self.phys.y
			if parent!=nil then
				dx+=parent.phys.x
				dy+=parent.phys.y
			end
			local sp=types[self.id].sp
			spr(sp,dx-self.rad,dy-self.rad)
		end,
		add_collisions=function (self)
			-- todo
		end
	}
end

function init_sim()
	sim_actors={}
	
	for a in all(bmb.cld) do
		local len=dist(bmb.x,bmb.y,a.x,a.y)
	
		if #a.cld==0 then
			local na=make_actor(
				a.id,a.x,a.y,
				(a.x-bmb.x)/len,
			 (a.y-bmb.y)/len,
			 2.5, 
			 4)
			add(sim_actors,na)
		else
			-- todo
		end
	end
	
	colliders={}
	register_aabb(30,30,20,30)
end

function tick_sim()
	collisions={}
	for ent in all(sim_actors) do
		ent.phys:advance()
		ent:add_collisions(collisions)
	end
	
end

function draw_sim()
	for ent in all(sim_actors) do
		ent:draw()
	end
	
	debug_phys()
end
-->8
-- util

-- disable repeat on btnp
poke(0x5f5c,255)

function clamp(val,minv,maxv)
	return min(maxv,max(minv,val))
end

function length(ax,ay)
	return dist(0,0,ax,ay)
end

function dist(ax,ay,bx,by)
	local dx=bx-ax
	local dy=by-ay
	return sqrt(dx*dx+dy*dy)
end

dbgstrs={}
function dbg(str)
	add(dbgstrs,str)
end

function draw_debug()
	camera()
	print("cpu: "..stat(1),0,0)
	for s in all(dbgstrs) do
		print(s)
	end
	dbgstrs={}
end

-->8
-- state dispatch

gstate=1
states={
	{menu_update, menu_draw},
	{game_update, game_draw}
}

function _update()
	states[gstate][1]()
end

function _draw()
	states[gstate][2]()
end
-->8
--physics

_compass={
	{x=0,y=1},
	{x=1,y=0},
	{x=0,y=-1},
	{x=-1,y=0}
}

function normalize(vec)
	local l=length(vec.x,vec.y)
	return {
		x=vec.x/l,
		y=vec.y/l
	}
end

function dot(v1,v2)
	return v1.x*v2.x + v1.y*v2.y
end

function vec_dir(diff)
	local mx=0
	local best=1
	local norm=normalize(diff)
	
	for i=1,4 do
		local dp=dot(norm,_compass[i])
		if dp > mx then
			mx = dp
			best = i
		end
	end
	
	return _compass[best]
end

function chk_coll_aabb(aabb, crcl)
	local cx=crcl.x
	local cy=crcl.y
	local bcx=aabb.x+aabb.hw
	local bcy=aabb.y+aabb.hh
	local dx=cx-bcx
	local dy=cy-bcy
	
	local clampx=clamp(dx,-aabb.hw,aabb.hw)
	local clampy=clamp(dy,-aabb.hh,aabb.hh)
	
	local closex=bcx+clampx
	local closey=bcy+clampy
	
	dx=closex-cx
	dy=closey-cy
	
	local dst=length(dx,dy)

	if dst < crcl.rad then
		return {
			diffx=dx,
			diffy=dy,
			hitx=closex,
			hity=closey,
			dir=vec_dir({x=-dx,y=-dy})
		}
	else
		return nil
	end
end
__gfx__
000b000044444444333333330a000000000000000000000000000000000000000000000000000000000e80000000000000000888888000000000000000000000
000b00004444444433333333a9a0000000000000000000000000000000000000000000000000000008e5ee800000000000088ffffff880000000000000000000
0000000044444444333333330a0000000000000000000000000000000000000000000000000000000e5555e000000000008ffffffffff8000000066600000000
bb000bb04444444433333333000000000000000000000000000000000000000000000000000000008e55555e0000000008ffffeeeeffff800000677700000000
00000000444444443333333300000000000000000000000000000000000000000000000000000000e55555e80000000008feeeffffeeef800006777700000000
000b00004f4f4f4f33333333000000000000000000000000000000000000000000000000000000000e5555e0000000008ffffffffffffff80067777700000000
000b0000f4f4f4f4333333330000000000000000000000000000000000000000000000000000000008ee5e80000000008f888f8ff8f888f80067777700000000
00000000ffffffff33333333000000000000000000000000000000000000000000000000000000000008e000000000008ff8ff88f8ff8ff80067777700000000
44444f4fffffffffff444444444444444444444444444f4ff4f4444400000000000000000000000000000000000000008ff8ff8f88ff8ff80000000000000000
444444ff4f4f4f4ff4f444444444444444444444444444f44f44444400000000000000000000000000000000000000008ff8ff8ff8ff8ff80000000000000000
44444f4ff4f4f4f4ff44444444444444444444444444444ff444444400000000000000000000000000000000000000008ffffffffffffff80000000000000000
444444ff44444444f4f4444444444444444444444444444444444444000000000000000000000000000000000000000008feeeffffeeef800000000000000000
44444f4f44444444ff44444444444444444444444444444444444444000000000000000000000000000000000000000008ffffeeeeffff800000000000000000
444444ff44444444f4f44444f44444444444444f44444444444444440000000000000000000000000000000000000000008ffffffffff8000000000000000000
44444f4f44444444ff4444444f444444444444f44444444444444444000000000000000000000000000000000000000000088ffffff880000000000000000000
444444ff44444444f4f44444f4f4444444444f4f4444444444444444000000000000000000000000000000000000000000000888888000000000000000000000
__map__
1401010101010101010101010101011300000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1002020202020202020202020202021200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
1511111111111111111111111111111600000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

#include <git_common.h>

#include <git_api.h>

extern "C" int progress_cb(const char *str, int len, void *data) {
	(void)data;

	char *progress_str = new char[len + 1];
	memcpy(progress_str, str, len);
	progress_str[len] = '\0';
	godot::Godot::print("remote: " + godot::String(progress_str).strip_edges());
	delete[] progress_str;

	return 0;
}

extern "C" int update_cb(const char *refname, const git_oid *a, const git_oid *b, void *data) {
	constexpr int short_commit_length = 8;
	char a_str[short_commit_length + 1];
	char b_str[short_commit_length + 1];
	(void)data;

	git_oid_tostr(b_str, short_commit_length, b);
	if (git_oid_is_zero(a)) {
		godot::Godot::print("* [new] " + godot::String(b_str) + " " + godot::String(refname));
	} else {
		git_oid_tostr(a_str, short_commit_length, a);
		godot::Godot::print("[updated] " + godot::String(a_str) + "..." + godot::String(b_str) + " " + godot::String(refname));
	}

	return 0;
}

extern "C" int transfer_progress_cb(const git_indexer_progress *stats, void *payload) {
	(void)payload;

	if (stats->received_objects == stats->total_objects) {
		godot::Godot::print("Resolving deltas " + godot::String::num_int64(stats->indexed_deltas) + "/" + godot::String::num_int64(stats->total_deltas));
	} else if (stats->total_objects > 0) {
		godot::Godot::print("Received " + godot::String::num_int64(stats->received_objects) + "/" + godot::String::num_int64(stats->total_objects) + " objects (" + godot::String::num_int64(stats->indexed_objects) + ") in " + godot::String::num_int64(static_cast<int>(stats->received_bytes)) + " bytes");
	}
	return 0;
}

extern "C" int fetchhead_foreach_cb(const char *ref_name, const char *remote_url, const git_oid *oid, unsigned int is_merge, void *payload) {
	if (is_merge) {
		git_oid_cpy((git_oid *)payload, oid);
	}
	return 0;
}

extern "C" int push_transfer_progress_cb(unsigned int current, unsigned int total, size_t bytes, void *payload) {
	int64_t progress = 100;

	if (total != 0) {
		progress = (current * 100) / total;
	}

	godot::Godot::print("Writing Objects: " +
						godot::String::num_int64(progress) + "% (" +
						godot::String::num_int64((int)current) + "/" + godot::String::num_int64((int)total) + "), " + godot::String::num_int64(bytes) + " bytes, done.");
	return 0;
}

extern "C" int push_update_reference_cb(const char *refname, const char *status, void *data) {
	godot::String status_str = status;
	if (status_str == "") {
		godot::Godot::print("[rejected] " + godot::String(refname) + " " + status_str);
	} else {
		godot::Godot::print("[updated] " + godot::String(refname));
	}
	return 0;
}

extern "C" int credentials_cb(git_cred **out, const char *url, const char *username_from_url, unsigned int allowed_types, void *payload) {
	Credentials *creds = (Credentials *)payload;

	godot::String proper_username = username_from_url ? username_from_url : creds->username;

	if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
		return git_cred_userpass_plaintext_new(out, godot::CString(proper_username).data, godot::CString(creds->password).data);
	}

	if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
		return git_credential_ssh_key_new(out,
				godot::CString(proper_username).data,
				godot::CString(creds->ssh_public_key_path).data,
				godot::CString(creds->ssh_private_key_path).data,
				godot::CString(creds->ssh_passphrase).data);
	}

	if (allowed_types & GIT_CREDENTIAL_USERNAME) {
		return git_credential_username_new(out, godot::CString(proper_username).data);
	}

	return GIT_EUSER;
}

extern "C" int diff_hunk_cb(const git_diff_delta *delta, const git_diff_hunk *range, void *payload) {
	godot::DiffHelper *diff_helper = (godot::DiffHelper *)payload;

	godot::Dictionary hunk = diff_helper->git_api->create_diff_hunk(range->old_start, range->new_start, range->old_lines, range->new_lines);
	diff_helper->diff_hunks->push_back(hunk);

	return 1;
}
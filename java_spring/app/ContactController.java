import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import org.springframework.data.domain.PageRequest;

import java.util.List;

@RestController
@RequestMapping("/contacts")
public class ContactController {

    @Autowired
    private ContactRepository contactRepository;

    @PostMapping
    public Contact createContact(@RequestBody Contact contact) {
        contact.setDateCreated(LocalDateTime.now());
        contact.setDateUpdated(LocalDateTime.now());
        return contactRepository.save(contact);
    }

    @GetMapping
    public List<Contact> getContacts(
            @RequestParam(required = false) Integer externalId,
            @RequestParam(required = false) String phoneNumber,
            @RequestParam(defaultValue = "10000") int limit,
            @RequestParam(defaultValue = "0") int offset) {

        return contactRepository.findByFilters(externalId, phoneNumber, PageRequest.of(offset / limit, limit));
    }
}
